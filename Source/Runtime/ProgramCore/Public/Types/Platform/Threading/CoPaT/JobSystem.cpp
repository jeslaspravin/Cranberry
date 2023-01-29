/*!
 * \file JobSystem.cpp
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "JobSystem.h"
#include "Platform/PlatformThreadingFunctions.h"

#include <thread>

COPAT_NS_INLINED
namespace copat
{

void getCoreCount(u32 &outCoreCount, u32 &outLogicalProcessorCount)
{
    PlatformThreadingFuncs::getCoreCount(outCoreCount, outLogicalProcessorCount);
    // Just a backup if user did not provide an implementation
    if (outCoreCount == 0 || outLogicalProcessorCount == 0)
    {
        outLogicalProcessorCount = std::thread::hardware_concurrency();
        outCoreCount = outLogicalProcessorCount / 2;
    }
}

JobSystem *JobSystem::singletonInstance = nullptr;

void JobSystem::initialize(MainThreadTickFunc &&mainTick, void *inUserData)
{
    COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatInit"));

    if (!singletonInstance)
    {
        singletonInstance = this;
    }

    if (!PlatformThreadingFuncs::createTlsSlot(tlsSlot))
    {
        return;
    }

    specialThreadsPool.initialize(this);

    initializeWorkers();

    // Setup main thread
    mainThreadTick = std::forward<MainThreadTickFunc>(mainTick);
    userData = inUserData;
    PlatformThreadingFuncs::setCurrentThreadName(COPAT_TCHAR("MainThread"));
    PlatformThreadingFuncs::setCurrentThreadProcessor(0, 0);
    PerThreadData &mainThreadData = getOrCreatePerThreadData();
    mainThreadData.threadType = EJobThreadType::MainThread;
}

void JobSystem::initializeWorkers()
{
    COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatWorkerThreadsInit"));

    u32 coreCount, logicalProcCount;
    getCoreCount(coreCount, logicalProcCount);
    const u32 htCount = logicalProcCount / coreCount;

    u32 nonWorkerCount = SpecialThreadsPoolType::COUNT + 1;
    u32 coresForWorkers = coreCount;
    if (coreCount <= nonWorkerCount)
    {
        // If there is very limited cores then let OS decide on scheduling, we just distribute workers evenly
        nonWorkerCount = 0;
    }
    else
    {
        coresForWorkers = coreCount - nonWorkerCount;
    }

    for (u32 i = 0; i < workersCount; ++i)
    {
        u32 coreIdx = (i % coresForWorkers) + nonWorkerCount;
        u32 htIdx = (i / coresForWorkers) % htCount;

        // Create and setup thread
        std::thread worker{ [this]()
                            {
                                doWorkerJobs();
                            } };
        PlatformThreadingFuncs::setThreadName((COPAT_TCHAR("WorkerThread_") + COPAT_TOSTRING(i)).c_str(), worker.native_handle());
        PlatformThreadingFuncs::setThreadProcessor(coreIdx, htIdx, worker.native_handle());
        // Destroy when finishes
        worker.detach();
    }
}

void JobSystem::shutdown()
{
    COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatShutdown"));

    PerThreadData *mainThreadTlData = getPerThreadData();
    COPAT_ASSERT(mainThreadTlData->threadType == EJobThreadType::MainThread);

    // Just setting bExitMain flag to expected when shutting down
    bExitMain[0].test_and_set(std::memory_order::relaxed);
    bExitMain[1].test_and_set(std::memory_order::release);

    specialThreadsPool.shutdown();

    // Binary semaphore
    // while (!workersFinishedEvent.try_wait())
    //{
    //    workerJobEvent.release();
    //}

    // Counting semaphore
    // Drain the worker job events if any so we can release all workers
    while (workerJobEvent.try_acquire())
    {}
    workerJobEvent.release(workersCount);
    workersFinishedEvent.wait();

    memDelete(mainThreadTlData);
    if (singletonInstance == this)
    {
        singletonInstance = nullptr;
    }
}

void JobSystem::enqueueJob(std::coroutine_handle<> coro, EJobThreadType enqueueToThread /*= EJobThreadType::WorkerThreads*/)
{
    PerThreadData *threadData = getPerThreadData();

    if (enqueueToThread == EJobThreadType::MainThread)
    {
        COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatEnqueueToMain"));

        if (threadData)
        {
            mainThreadJobs.enqueue(coro.address(), threadData->mainEnqToken);
        }
        else
        {
            mainThreadJobs.enqueue(coro.address());
        }
    }
    else if (enqueueToThread == EJobThreadType::WorkerThreads)
    {
        COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatEnqueueToWorker"));
        COPAT_ASSERT(!workersFinishedEvent.try_wait());

        if (threadData)
        {
            workerJobs.enqueue(coro.address(), threadData->workerEnqDqToken);
        }
        else
        {
            workerJobs.enqueue(coro.address());
        }
        // We do not have to be very strict here as long as one or two is free and we get 0 or nothing is free and we release one or two
        // more it is fine
        if (availableWorkersCount.load(std::memory_order::relaxed) != 0)
        {
            workerJobEvent.release();
        }
    }
    else
    {
        COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatEnqueueToSpecial"));
        if (threadData)
        {
            specialThreadsPool.enqueueJob(coro, enqueueToThread, threadData->specialThreadTokens);
        }
        else
        {
            specialThreadsPool.enqueueJob(coro, enqueueToThread, nullptr);
        }
    }
}

copat::JobSystem::PerThreadData &JobSystem::getOrCreatePerThreadData()
{
    PerThreadData *threadData = (PerThreadData *)PlatformThreadingFuncs::getTlsSlotValue(tlsSlot);
    if (!threadData)
    {
        PerThreadData *newThreadData = memNew<PerThreadData>(workerJobs.getHazardToken(), mainThreadJobs.getHazardToken(), specialThreadsPool);
        PlatformThreadingFuncs::setTlsSlotValue(tlsSlot, newThreadData);
        threadData = (PerThreadData *)PlatformThreadingFuncs::getTlsSlotValue(tlsSlot);
    }
    return *threadData;
}

void JobSystem::runMain()
{
    // Main thread data gets created and destroy in initialize and shutdown resp.
    PerThreadData *tlData = getPerThreadData();
    tlData->threadType = EJobThreadType::MainThread;
    while (true)
    {
        COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatMainTick"));
        if (bool(mainThreadTick))
        {
            mainThreadTick(userData);
        }

        while (void *coroPtr = mainThreadJobs.dequeue())
        {
            COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatMainJob"));
            std::coroutine_handle<>::from_address(coroPtr).resume();
        }

        if (bExitMain[0].test(std::memory_order::relaxed))
        {
            break;
        }
    }
}

void JobSystem::doWorkerJobs()
{
    PerThreadData *tlData = &getOrCreatePerThreadData();
    tlData->threadType = EJobThreadType::WorkerThreads;
    while (true)
    {
        while (void *coroPtr = workerJobs.dequeue(tlData->workerEnqDqToken))
        {
            COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatWorkerJob"));
            std::coroutine_handle<>::from_address(coroPtr).resume();
        }

        if (bExitMain[1].test(std::memory_order::relaxed))
        {
            break;
        }

        // Marking that we are becoming available
        availableWorkersCount.fetch_add(1, std::memory_order::memory_order_acq_rel);
        // Wait until job available
        workerJobEvent.acquire();
        // Starting work
        availableWorkersCount.fetch_sub(1, std::memory_order::memory_order_acq_rel);
    }
    workersFinishedEvent.count_down();
    memDelete(tlData);
}

copat::u32 JobSystem::calculateWorkersCount() const
{
    u32 coreCount, logicalProcCount;
    getCoreCount(coreCount, logicalProcCount);
    coreCount = coreCount > 4 ? coreCount : 4;
    return coreCount > MAX_SUPPORTED_WORKERS ? MAX_SUPPORTED_WORKERS : coreCount;
}

copat::JobSystem::PerThreadData *JobSystem::getPerThreadData() const
{
    return (PerThreadData *)PlatformThreadingFuncs::getTlsSlotValue(tlsSlot);
}

void INTERNAL_initializeAndRunSpecialThread(INTERNAL_SpecialThreadFuncType threadFunc, EJobThreadType threadType, JobSystem *jobSystem)
{
    u32 coreCount, logicalProcCount;
    getCoreCount(coreCount, logicalProcCount);

    std::thread specialThread{ [jobSystem, threadFunc]()
                               {
                                   (jobSystem->*threadFunc)();
                               } };
    PlatformThreadingFuncs::setThreadName(
        (COPAT_TCHAR("SpecialThread_") + COPAT_TOSTRING(u32(threadType))).c_str(), specialThread.native_handle()
    );
    if (coreCount > u32(threadType))
    {
        // If not enough core just run as free thread
        PlatformThreadingFuncs::setThreadProcessor(u32(threadType), 0, specialThread.native_handle());
    }
    // Destroy when finishes
    specialThread.detach();
}

} // namespace copat