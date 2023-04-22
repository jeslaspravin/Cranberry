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
void INTERNAL_initializeAndRunSpecialThread(
    INTERNAL_SpecialThreadFuncType threadFunc, EJobThreadType threadType, u32 threadIdx, JobSystem *jobSystem
);

struct alignas(2 * CACHE_LINE_SIZE) SpecialJobReceivedEvent
{
    std::atomic_flag flag;

    void notify() noexcept
    {
        flag.test_and_set(std::memory_order::release);
        flag.notify_one();
    }

    void wait() noexcept
    {
        flag.wait(false, std::memory_order::acquire);
        // Relaxed is fine this will not be reordered before wait acquire
        flag.clear(std::memory_order::relaxed);
    }
};

#define SPECIALTHREAD_NAME_FIRST(ThreadType) COPAT_TCHAR(#ThreadType)
#define SPECIALTHREAD_NAME(ThreadType) , COPAT_TCHAR(#ThreadType)

template <u32 SpecialThreadsCount>
class SpecialThreadsPool
{
public:
    constexpr static const u32 COUNT = SpecialThreadsCount;
    constexpr static const TChar *NAMES[]
        = { FOR_EACH_UDTHREAD_TYPES_UNIQUE_FIRST_LAST(SPECIALTHREAD_NAME_FIRST, SPECIALTHREAD_NAME, SPECIALTHREAD_NAME) };

    JobSystem *ownerJobSystem = nullptr;

    // It is okay to have as array as each queue will be aligned 2x the Cache line size
    SpecialThreadQueueType specialQueues[COUNT * Priority_MaxPriority];
    SpecialJobReceivedEvent specialJobEvents[COUNT];
    std::latch allSpecialsFinishedEvent{ COUNT };

private:
    struct EnqueueTokensAllocator
    {
        SpecialQHazardToken *hazardTokens{ nullptr };
        u32 totalTokens{ 0 };
        std::atomic_int_fast32_t stackTop{ 0 };

        void initialize(u32 totalThreads) noexcept;
        void release() noexcept;

        SpecialQHazardToken *allocate() noexcept;
    };

    EnqueueTokensAllocator tokensAllocator;

public:
    void initialize(JobSystem *jobSystem) noexcept;
    void shutdown() noexcept;

    void enqueueJob(
        std::coroutine_handle<> coro, EJobThreadType enqueueToThread, EJobPriority priority, SpecialQHazardToken *fromThreadTokens
    ) noexcept
    {
        // We must not enqueue at shutdown
        COPAT_ASSERT(!allSpecialsFinishedEvent.try_wait());
        const u32 threadIdx = threadTypeToIdx(enqueueToThread);
        const u32 queueArrayIdx = pAndTTypeToIdx(threadIdx, priority);
        if (fromThreadTokens)
        {
            specialQueues[queueArrayIdx].enqueue(coro.address(), fromThreadTokens[queueArrayIdx]);
        }
        else
        {
            specialQueues[queueArrayIdx].enqueue(coro.address());
        }

        specialJobEvents[threadIdx].notify();
    }

    SpecialThreadQueueType *getThreadJobsQueue(u32 threadIdx, EJobPriority priority) noexcept
    {
        return &specialQueues[pAndTTypeToIdx(threadIdx, priority)];
    }
    SpecialJobReceivedEvent *getJobEvent(u32 threadIdx) noexcept { return &specialJobEvents[threadIdx]; }
    void onSpecialThreadExit() noexcept { allSpecialsFinishedEvent.count_down(); }

    /**
     * Allocates COUNT number of enqueue tokens, One for each special thread to be used for en queuing job from threads for each priority
     */
    SpecialQHazardToken *allocateEnqTokens() noexcept
    {
        SpecialQHazardToken *tokens = tokensAllocator.allocate();
        // Tokens can be null if special thread is disabled
        if (tokens == nullptr)
        {
            return tokens;
        }

        for (u32 threadIdx = 0; threadIdx < COUNT; ++threadIdx)
        {
            for (EJobPriority priority = Priority_Critical; priority < Priority_MaxPriority; priority = EJobPriority(priority + 1))
            {
                new (tokens + pAndTTypeToIdx(threadIdx, priority))
                    SpecialQHazardToken(getThreadJobsQueue(threadIdx, priority)->getHazardToken());
            }
        }
        return tokens;
    }

private:
    static constexpr u32 threadTypeToIdx(EJobThreadType threadType) { return u32(threadType) - (u32(EJobThreadType::MainThread) + 1); }
    static constexpr EJobThreadType idxToThreadType(u32 threadIdx) { return EJobThreadType(threadIdx + 1 + u32(EJobThreadType::MainThread)); }
    // Index to job priority and thread type index
    static constexpr u32 idxToTTypeAndP(u32 idx, EJobPriority &outPriority)
    {
        outPriority = idx % Priority_MaxPriority;
        return idx / Priority_MaxPriority;
    }
    // Priority and thread type idx combined to get index in linear array
    static constexpr u32 pAndTTypeToIdx(u32 threadIdx, EJobPriority priority) { return threadIdx * Priority_MaxPriority + priority; }

    template <u32 Idx>
    void initializeSpecialThread() noexcept;
    template <u32... Indices>
    void initializeSpecialThreads(std::integer_sequence<u32, Indices...>) noexcept;
};

#undef SPECIALTHREAD_NAME_FIRST
#undef SPECIALTHREAD_NAME

/**
 * For no special threads case
 */
template <>
class SpecialThreadsPool<0>
{
public:
    constexpr static const u32 COUNT = 0;
    constexpr static const TChar *NAMES[] = { COPAT_TCHAR("Dummy") };

    void initialize(JobSystem *) {}
    void shutdown() {}

    void enqueueJob(std::coroutine_handle<>, EJobThreadType, EJobPriority, SpecialQHazardToken *) {}

    SpecialThreadQueueType *getThreadJobsQueue(u32, EJobPriority) { return nullptr; }
    SpecialJobReceivedEvent *getJobEvent(u32) { return nullptr; }
    void onSpecialThreadExit() {}

    SpecialQHazardToken *allocateEnqTokens() { return nullptr; }
};

#define NOSPECIALTHREAD_ENUM_TO_FLAGBIT(ThreadType)                                                                                            \
    (copat::JobSystem::BitMasksStart << (copat::JobSystem::No##ThreadType - copat::JobSystem::BitMasksStart))

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
        WorkerQHazardToken workerEnqDqToken[Priority_MaxPriority];
        SpecialQHazardToken mainEnqToken[Priority_MaxPriority];
        SpecialQHazardToken *specialThreadTokens;

        PerThreadData(WorkerThreadQueueType *workerQs, SpecialThreadQueueType *mainQs, SpecialThreadsPoolType &specialThreadPool);
    };

#define NOSPECIALTHREAD_ENUM_FIRST(ThreadType) No##ThreadType = BitMasksStart,
#define NOSPECIALTHREAD_ENUM(ThreadType) No##ThreadType,
    // clang-format off

    // Allows controlling the threading model of the application at runtime
    enum EThreadingConstraint : u32
    {
        // Normal with all special and worker threads
        NoConstraints,
        // No worker or special threads, Only main thread exists
        SingleThreaded,
        NoSpecialThreads,
        NoWorkerThreads,
        // Anything after 8 will be bit masked values. Bit shift is determined by (Flag - BitMasksStart)
        BitMasksStart = 8,
        // Each of below NoSpecialThread mask will not stop creating those threads but will be used only at Enqueue. This is just to avoid unnecessary complexity
        FOR_EACH_UDTHREAD_TYPES_UNIQUE_FIRST_LAST(NOSPECIALTHREAD_ENUM_FIRST, NOSPECIALTHREAD_ENUM, NOSPECIALTHREAD_ENUM) 
        BitMasksEnd
    };

    // clang-format on
#undef NOSPECIALTHREAD_ENUM_FIRST
#undef NOSPECIALTHREAD_ENUM

private:
    static JobSystem *singletonInstance;

    u32 tlsSlot = 0;
    u32 threadingConstraints = EThreadingConstraint::NoConstraints;

    u32 workersCount;
    WorkerThreadQueueType workerJobs[Priority_MaxPriority];
    // Binary semaphore wont work if two jobs arrive at same time, and one of 2 just ends up waiting until another job arrives
    // std::binary_semaphore workerJobEvent{0};
    std::counting_semaphore<2 * MAX_SUPPORTED_WORKERS> workerJobEvent;
    // To ensure that we do not trigger workerJobEvent when there is no free workers
    std::atomic_uint_fast64_t availableWorkersCount;
    // For waiting until all workers are finished
    std::latch workersFinishedEvent;

    SpecialThreadQueueType mainThreadJobs[Priority_MaxPriority];
    // 0 will be used by main thread loop itself while 1 will be used by worker threads to run until shutdown is called
    std::atomic_flag bExitMain[2];
    // Main thread tick function type, This function gets ticked in main thread for every loop and then main job queue will be emptied
    MainThreadTickFunc mainThreadTick;
    void *userData = nullptr;

    SpecialThreadsPoolType specialThreadsPool;

    EJobThreadType enqIndirection[u32(EJobThreadType::MaxThreads)];

public:
    // EThreadingConstraint for constraints
    JobSystem(u32 constraints);
    JobSystem(u32 inWorkerCount, u32 constraints);

    static JobSystem *get() noexcept { return singletonInstance; }

    void initialize(MainThreadTickFunc &&mainTickFunc, void *inUserData) noexcept;
    void joinMain() noexcept { runMain(); }
    void exitMain() noexcept { bExitMain[0].test_and_set(std::memory_order::release); }
    void shutdown() noexcept;

    void enqueueJob(
        std::coroutine_handle<> coro, EJobThreadType enqueueToThread = EJobThreadType::WorkerThreads,
        EJobPriority priority = EJobPriority::Priority_Normal
    ) noexcept;

    EJobThreadType getCurrentThreadType() const noexcept
    {
        PerThreadData *tlData = getPerThreadData();
        if (tlData == nullptr)
        {
            return EJobThreadType::MaxThreads;
        }
        else
        {
            return tlData->threadType;
        }
    }
    EJobThreadType enqToThreadType(EJobThreadType forThreadType) const { return EJobThreadType(enqIndirection[u32(forThreadType)]); }
    bool isInThread(EJobThreadType threadType) const { return getCurrentThreadType() == enqToThreadType(threadType); }

    u32 getWorkersCount() const { return workersCount; }
    u32 getTotalThreadsCount() const { return workersCount + SpecialThreadsPoolType::COUNT + 1; }

private:
    void initializeWorkers() noexcept;

    PerThreadData *getPerThreadData() const noexcept;
    PerThreadData &getOrCreatePerThreadData() noexcept;

    u32 calculateWorkersCount() const noexcept;

    void runMain() noexcept;
    void doWorkerJobs() noexcept;
    template <u32 SpecialThreadIdx, EJobThreadType SpecialThreadType>
    void doSpecialThreadJobs() noexcept
    {
        PerThreadData *tlData = &getOrCreatePerThreadData();
        tlData->threadType = SpecialThreadType;
        while (true)
        {
            // Execute all tasks in Higher priority to lower priority order
            void *coroPtr = nullptr;
            for (EJobPriority priority = Priority_Critical; priority < Priority_MaxPriority && coroPtr == nullptr;
                 priority = EJobPriority(priority + 1))
            {
                coroPtr = specialThreadsPool.getThreadJobsQueue(SpecialThreadIdx, priority)->dequeue();
            }
            while (coroPtr)
            {
                COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatSpecialJob"));
                std::coroutine_handle<>::from_address(coroPtr).resume();

                coroPtr = nullptr;
                for (EJobPriority priority = Priority_Critical; priority < Priority_MaxPriority && coroPtr == nullptr;
                     priority = EJobPriority(priority + 1))
                {
                    coroPtr = specialThreadsPool.getThreadJobsQueue(SpecialThreadIdx, priority)->dequeue();
                }
            }

            if (bExitMain[1].test(std::memory_order::relaxed))
            {
                break;
            }

            specialThreadsPool.getJobEvent(SpecialThreadIdx)->wait();
        }
        specialThreadsPool.onSpecialThreadExit();

        CoPaTMemAlloc::memFree(tlData);
    }
};

//////////////////////////////////////////////////////////////////////////
/// SpecialThreadsPool impl
//////////////////////////////////////////////////////////////////////////

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::initialize(JobSystem *jobSystem) noexcept
{
    COPAT_ASSERT(jobSystem);
    COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatSpecialThreadsInit"));

    ownerJobSystem = jobSystem;

    tokensAllocator.initialize(ownerJobSystem->getTotalThreadsCount());
    initializeSpecialThreads(std::make_integer_sequence<u32, COUNT>{});
}

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::shutdown() noexcept
{
    COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatSpecialThreadsShutdown"));
    for (u32 i = 0; i < COUNT; ++i)
    {
        specialJobEvents[i].notify();
    }
    allSpecialsFinishedEvent.wait();
    tokensAllocator.release();
}

template <u32 SpecialThreadsCount>
template <u32 Idx>
void SpecialThreadsPool<SpecialThreadsCount>::initializeSpecialThread() noexcept
{
    constexpr static const EJobThreadType threadType = idxToThreadType(Idx);
    INTERNAL_SpecialThreadFuncType func = &JobSystem::doSpecialThreadJobs<Idx, threadType>;
    INTERNAL_initializeAndRunSpecialThread(func, threadType, Idx, ownerJobSystem);
}

template <u32 SpecialThreadsCount>
template <u32... Indices>
void SpecialThreadsPool<SpecialThreadsCount>::initializeSpecialThreads(std::integer_sequence<u32, Indices...>) noexcept
{
    (initializeSpecialThread<Indices>(), ...);
}

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::EnqueueTokensAllocator::initialize(u32 totalThreads) noexcept
{
    if (hazardTokens != nullptr)
    {
        CoPaTMemAlloc::memFree(hazardTokens);
        hazardTokens = nullptr;
    }
    totalTokens = totalThreads * COUNT * Priority_MaxPriority;
    hazardTokens = (SpecialQHazardToken *)(CoPaTMemAlloc::memAlloc(sizeof(SpecialQHazardToken) * totalTokens, alignof(SpecialQHazardToken)));
    stackTop.store(0, std::memory_order::relaxed);
}

template <u32 SpecialThreadsCount>
void SpecialThreadsPool<SpecialThreadsCount>::EnqueueTokensAllocator::release() noexcept
{
    if (hazardTokens != nullptr)
    {
        CoPaTMemAlloc::memFree(hazardTokens);
        hazardTokens = nullptr;
    }
}

template <u32 SpecialThreadsCount>
SpecialQHazardToken *SpecialThreadsPool<SpecialThreadsCount>::EnqueueTokensAllocator::allocate() noexcept
{
    u32 tokenIdx = stackTop.fetch_add(COUNT * Priority_MaxPriority, std::memory_order::acq_rel);
    if (tokenIdx < totalTokens)
    {
        return hazardTokens + tokenIdx;
    }
    return nullptr;
}

} // namespace copat