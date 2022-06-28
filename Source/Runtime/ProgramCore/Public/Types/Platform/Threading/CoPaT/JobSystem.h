/*!
 * \file JobSystem.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "FAAArrayQueue.hpp"

#include <coroutine>
#include <semaphore>
#include <latch>

COPAT_NS_INLINED
namespace copat
{
class JobSystem;

using SpecialThreadQueueType = FAAArrayMPSCQueue<void>;
using SpecialQHazardToken = SpecialThreadQueueType::HazardToken;
using WorkerThreadQueueType = FAAArrayQueue<void>;
using WorkerQHazardToken = WorkerThreadQueueType::HazardToken;

/**
 * Just to not leak thread include
 */
using INTERNAL_SpecialThreadFuncType = void (JobSystem::*)();
void INTERNAL_initializeAndRunSpecialThread(INTERNAL_SpecialThreadFuncType threadFunc, EJobThreadType threadType, JobSystem *jobSystem);

struct alignas(2 * CACHE_LINE_SIZE) SpecialJobReceivedEvent
{
    std::atomic_flag flag;

    void notify()
    {
        flag.test_and_set(std::memory_order::release);
        flag.notify_one();
    }

    void wait()
    {
        flag.wait(false, std::memory_order::acquire);
        // Relaxed is fine this will not be reordered before wait acquire
        flag.clear(std::memory_order::relaxed);
    }
};

template <u32 SpecialThreadsCount>
class SpecialThreadsPool
{
public:
    constexpr static const u32 COUNT = SpecialThreadsCount;
    JobSystem *ownerJobSystem = nullptr;

    // It is okay to have as array as each queue will be aligned 2x the Cache line size
    SpecialThreadQueueType specialQueues[COUNT];
    SpecialJobReceivedEvent specialJobEvents[COUNT];
    std::latch allSpecialsFinishedEvent{ COUNT };

private:
    struct EnqueueTokensAllocator
    {
        SpecialQHazardToken *hazardTokens{ nullptr };
        u32 totalTokens{ 0 };
        std::atomic_int_fast32_t stackTop{ 0 };

        void initialize(u32 totalThreads);
        void release();

        SpecialQHazardToken *allocate();
    };

    EnqueueTokensAllocator tokensAllocator;

public:
    void initialize(JobSystem *jobSystem);
    void shutdown();

    void enqueueJob(std::coroutine_handle<> coro, EJobThreadType enqueueToThread, SpecialQHazardToken *fromThreadTokens)
    {
        // We must not enqueue at shutdown
        COPAT_ASSERT(fromThreadTokens && !allSpecialsFinishedEvent.try_wait());
        const u32 idx = threadTypeToIdx(enqueueToThread);
        specialQueues[idx].enqueue(coro.address(), fromThreadTokens[idx]);

        specialJobEvents[idx].notify();
    }

    SpecialThreadQueueType *getThreadJobsQueue(u32 idx) { return &specialQueues[idx]; }
    SpecialJobReceivedEvent *getJobEvent(u32 idx) { return &specialJobEvents[idx]; }
    void onSpecialThreadExit() { allSpecialsFinishedEvent.count_down(); }

    /**
     * Allocates COUNT number of enqueue tokens, One for each special thread to be used for en queuing job from threads
     */
    SpecialQHazardToken *allocateEnqTokens()
    {
        SpecialQHazardToken *tokens = tokensAllocator.allocate();
        COPAT_ASSERT(tokens);
        for (u32 specialThreadIdx = 0; specialThreadIdx < COUNT; ++specialThreadIdx)
        {
            new (tokens + specialThreadIdx) SpecialQHazardToken(getThreadJobsQueue(specialThreadIdx)->getHazardToken());
        }
        return tokens;
    }

private:
    static constexpr u32 threadTypeToIdx(EJobThreadType threadType) { return u32(threadType) - (u32(EJobThreadType::MainThread) + 1); }
    static constexpr EJobThreadType idxToThreadType(u32 idx) { return EJobThreadType(idx + 1 + u32(EJobThreadType::MainThread)); }

    template <u32 LastIdx>
    void initializeSpecialThreads(std::integer_sequence<u32, LastIdx>);
    template <u32 FirstIdx, u32... Indices>
    void initializeSpecialThreads(std::integer_sequence<u32, FirstIdx, Indices...>);
};

/**
 * For no special threads case
 */
template <>
class SpecialThreadsPool<0>
{
public:
    constexpr static const u32 COUNT = 0;

    void initialize(JobSystem *jobSystem) {}
    void shutdown() {}

    void enqueueJob(std::coroutine_handle<> coro, EJobThreadType enqueueToThread, SpecialQHazardToken *fromThreadTokens) {}

    SpecialThreadQueueType *getThreadJobsQueue(u32 idx) { return nullptr; }
    SpecialJobReceivedEvent *getJobEvent(u32 idx) { return nullptr; }
    void onSpecialThreadExit() {}

    SpecialQHazardToken *allocateEnqTokens() { return nullptr; }
};

class COPAT_EXPORT_SYM JobSystem
{
public:
    constexpr static const u32 MAX_SUPPORTED_WORKERS = 128;

    using MainThreadTickFunc = FunctionType<void, void *>;
    using SpecialThreadsPoolType = SpecialThreadsPool<u32(EJobThreadType::WorkerThreads) - u32(EJobThreadType::MainThread) - 1>;
    friend SpecialThreadsPoolType;

    struct PerThreadData
    {
        EJobThreadType threadType;
        WorkerQHazardToken workerEnqDqToken;
        SpecialQHazardToken mainEnqToken;
        SpecialQHazardToken *specialThreadTokens;

        PerThreadData(WorkerQHazardToken &&workerQToken, SpecialQHazardToken &&mainQToken, SpecialThreadsPoolType &specialThreadPool)
            : threadType(EJobThreadType::WorkerThreads)
            , workerEnqDqToken(std::forward<WorkerThreadQueueType::HazardToken>(workerQToken))
            , mainEnqToken(std::forward<SpecialThreadQueueType::HazardToken>(mainQToken))
            , specialThreadTokens(specialThreadPool.allocateEnqTokens())
        {}
    };

private:
    static JobSystem *singletonInstance;

    u32 tlsSlot;

    u32 workersCount;
    WorkerThreadQueueType workerJobs;
    // Binary semaphore wont work if two jobs arrive at same time, and one of 2 just ends up waiting until another job arrives
    // std::binary_semaphore workerJobEvent{0};
    std::counting_semaphore<2 * MAX_SUPPORTED_WORKERS> workerJobEvent;
    // To ensure that we do not trigger workerJobEvent when there is no free workers
    std::atomic_uint_fast64_t availableWorkersCount;
    // For waiting until all workers are finished
    std::latch workersFinishedEvent;

    SpecialThreadQueueType mainThreadJobs;
    // 0 will be used by main thread loop itself while 1 will be used by worker threads to run until shutdown is called
    std::atomic_flag bExitMain[2];
    // Main thread tick function type, This function gets ticked in main thread for every loop and then main job queue will be emptied
    MainThreadTickFunc mainThreadTick;
    void *userData;

    SpecialThreadsPoolType specialThreadsPool;

public:
    JobSystem()
        : workersCount(calculateWorkersCount())
        , workerJobEvent(0)
        , availableWorkersCount(0)
        , workersFinishedEvent(workersCount)
    {}

    static JobSystem *get() { return singletonInstance; }

    void initialize(MainThreadTickFunc &&mainTickFunc, void *inUserData);
    void joinMain() { runMain(); }
    void exitMain() { bExitMain[0].test_and_set(std::memory_order::release); }
    void shutdown();

    void enqueueJob(std::coroutine_handle<> coro, EJobThreadType enqueueToThread = EJobThreadType::WorkerThreads);

    EJobThreadType getCurrentThreadType() const
    {
        PerThreadData *tlData = getPerThreadData();
        COPAT_ASSERT(tlData);
        return tlData->threadType;
    }
    u32 getWorkersCount() const { return workersCount; }
    u32 getTotalThreadsCount() const { return workersCount + SpecialThreadsPoolType::COUNT + 1; }

private:
    void initializeWorkers();

    PerThreadData *getPerThreadData() const;
    PerThreadData &getOrCreatePerThreadData();

    u32 calculateWorkersCount() const;

    void runMain();
    void doWorkerJobs();
    template <u32 SpecialThreadIdx, EJobThreadType SpecialThreadType>
    void doSpecialThreadJobs()
    {
        PerThreadData *tlData = &getOrCreatePerThreadData();
        tlData->threadType = SpecialThreadType;
        while (true)
        {
            while (void *coroPtr = specialThreadsPool.getThreadJobsQueue(SpecialThreadIdx)->dequeue())
            {
                std::coroutine_handle<>::from_address(coroPtr).resume();
            }

            if (bExitMain[1].test(std::memory_order::relaxed))
            {
                break;
            }

            specialThreadsPool.getJobEvent(SpecialThreadIdx)->wait();
        }
        specialThreadsPool.onSpecialThreadExit();
        memDelete(tlData);
    }
};

//////////////////////////////////////////////////////////////////////////
/// SpecialThreadsPool impl
//////////////////////////////////////////////////////////////////////////

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::initialize(JobSystem *jobSystem)
{
    COPAT_ASSERT(jobSystem);
    ownerJobSystem = jobSystem;

    tokensAllocator.initialize(ownerJobSystem->getTotalThreadsCount());
    initializeSpecialThreads(std::make_integer_sequence<u32, COUNT>{});
}

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::shutdown()
{
    for (u32 i = 0; i < COUNT; ++i)
    {
        specialJobEvents[i].notify();
    }
    allSpecialsFinishedEvent.wait();
    tokensAllocator.release();
}

template <u32 SpecialThreadsCount>
template <u32 LastIdx>
void SpecialThreadsPool<SpecialThreadsCount>::initializeSpecialThreads(std::integer_sequence<u32, LastIdx>)
{
    constexpr static const EJobThreadType threadType = idxToThreadType(LastIdx);
    INTERNAL_SpecialThreadFuncType func = &JobSystem::doSpecialThreadJobs<LastIdx, threadType>;
    INTERNAL_initializeAndRunSpecialThread(func, threadType, ownerJobSystem);
}

template <u32 SpecialThreadsCount>
template <u32 FirstIdx, u32... Indices>
void SpecialThreadsPool<SpecialThreadsCount>::initializeSpecialThreads(std::integer_sequence<u32, FirstIdx, Indices...>)
{
    constexpr static const EJobThreadType threadType = idxToThreadType(FirstIdx);
    INTERNAL_SpecialThreadFuncType func = &JobSystem::doSpecialThreadJobs<FirstIdx, threadType>;
    INTERNAL_initializeAndRunSpecialThread(func, threadType, ownerJobSystem);
    initializeSpecialThreads(std::integer_sequence<u32, Indices...>{});
}

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::EnqueueTokensAllocator::initialize(u32 totalThreads)
{
    if (hazardTokens != nullptr)
    {
        CoPaTMemAlloc::memFree(hazardTokens);
        hazardTokens = nullptr;
    }
    totalTokens = totalThreads;
    hazardTokens
        = (SpecialQHazardToken *)(CoPaTMemAlloc::memAlloc(sizeof(SpecialQHazardToken) * totalThreads * COUNT, alignof(SpecialQHazardToken)));
    stackTop.store(0, std::memory_order::relaxed);
}

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::EnqueueTokensAllocator::release()
{
    if (hazardTokens != nullptr)
    {
        CoPaTMemAlloc::memFree(hazardTokens);
        hazardTokens = nullptr;
    }
}

template <u32 SpecialThreadsCount>
SpecialQHazardToken *SpecialThreadsPool<SpecialThreadsCount>::EnqueueTokensAllocator::allocate()
{
    u32 tokenIdx = stackTop.fetch_add(COUNT, std::memory_order::acq_rel);
    if (tokenIdx < totalTokens)
    {
        return hazardTokens + tokenIdx;
    }
    return nullptr;
}

} // namespace copat