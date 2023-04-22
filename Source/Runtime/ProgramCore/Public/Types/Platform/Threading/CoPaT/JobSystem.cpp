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

JobSystem::EThreadingConstraint getThreadingConstraint(u32 constraints)
{
    return JobSystem::EThreadingConstraint(constraints & (JobSystem::BitMasksStart - 1));
}

JobSystem *JobSystem::singletonInstance = nullptr;

JobSystem::JobSystem(u32 constraints)
    : threadingConstraints(constraints)
    , workersCount(calculateWorkersCount())
    , workerJobEvent(0)
    , availableWorkersCount(0)
    , workersFinishedEvent(workersCount)
{
    for (u32 i = 0; i < u32(EJobThreadType::MaxThreads); ++i)
    {
        enqIndirection[i] = EJobThreadType(i);
    }
}
JobSystem::JobSystem(u32 inWorkerCount, u32 constraints)
    : threadingConstraints(constraints)
    , workersCount(inWorkerCount)
    , workerJobEvent(0)
    , availableWorkersCount(0)
    , workersFinishedEvent(workersCount)
{
    for (u32 i = 0; i < u32(EJobThreadType::MaxThreads); ++i)
    {
        enqIndirection[i] = EJobThreadType(i);
    }
}

#define NO_SPECIALTHREADS_INDIR_SETUP(ThreadType) enqIndirection[u32(EJobThreadType::##ThreadType)] = EJobThreadType::MainThread;
#define SPECIALTHREAD_INDIR_SETUP(ThreadType)                                                                                                  \
    enqIndirection[u32(EJobThreadType::##ThreadType)]                                                                                          \
        = (threadingConstraints & NOSPECIALTHREAD_ENUM_TO_FLAGBIT(ThreadType)) ? EJobThreadType::MainThread : EJobThreadType::##ThreadType;

void JobSystem::initialize(MainThreadTickFunc &&mainTick, void *inUserData) noexcept
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

    EThreadingConstraint tConstraint = getThreadingConstraint(threadingConstraints);

    // Setup special threads
    if (tConstraint == EThreadingConstraint::SingleThreaded || tConstraint == EThreadingConstraint::NoSpecialThreads)
    {
        FOR_EACH_UDTHREAD_TYPES(NO_SPECIALTHREADS_INDIR_SETUP);
    }
    else
    {
        FOR_EACH_UDTHREAD_TYPES(SPECIALTHREAD_INDIR_SETUP);
        specialThreadsPool.initialize(this);
    }

    if (tConstraint == EThreadingConstraint::SingleThreaded || tConstraint == EThreadingConstraint::NoWorkerThreads)
    {
        enqIndirection[u32(EJobThreadType::WorkerThreads)] = EJobThreadType::MainThread;
    }
    else
    {
        initializeWorkers();
    }

    // Setup main thread
    mainThreadTick = std::forward<MainThreadTickFunc>(mainTick);
    userData = inUserData;
    PlatformThreadingFuncs::setCurrentThreadName(COPAT_TCHAR("MainThread"));
    PlatformThreadingFuncs::setCurrentThreadProcessor(0, 0);
    PerThreadData &mainThreadData = getOrCreatePerThreadData();
    mainThreadData.threadType = EJobThreadType::MainThread;
}
#undef NO_SPECIALTHREADS_INDIR_SETUP
#undef SPECIALTHREAD_INDIR_SETUP

void JobSystem::initializeWorkers() noexcept
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

void JobSystem::shutdown() noexcept
{
    COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatShutdown"));

    PerThreadData *mainThreadTlData = getPerThreadData();
    COPAT_ASSERT(mainThreadTlData->threadType == EJobThreadType::MainThread);

    // Just setting bExitMain flag to expected when shutting down
    bExitMain[0].test_and_set(std::memory_order::relaxed);
    bExitMain[1].test_and_set(std::memory_order::release);

    EThreadingConstraint tConstraint = getThreadingConstraint(threadingConstraints);

    if (tConstraint != EThreadingConstraint::SingleThreaded && tConstraint != EThreadingConstraint::NoSpecialThreads)
    {
        specialThreadsPool.shutdown();
    }

    if (tConstraint != EThreadingConstraint::SingleThreaded && tConstraint != EThreadingConstraint::NoWorkerThreads)
    {
        // Counting semaphore
        // Drain the worker job events if any so we can release all workers
        while (workerJobEvent.try_acquire())
        {}
        workerJobEvent.release(workersCount);
        workersFinishedEvent.wait();
    }

    memDelete(mainThreadTlData);
    if (singletonInstance == this)
    {
        singletonInstance = nullptr;
    }
}

void JobSystem::enqueueJob(
    std::coroutine_handle<> coro, EJobThreadType enqueueToThread /*= EJobThreadType::WorkerThreads*/,
    EJobPriority priority /*= EJobPriority::Priority_Normal*/
) noexcept
{
    PerThreadData *threadData = getPerThreadData();
    enqueueToThread = enqToThreadType(enqueueToThread);

    if (enqueueToThread == EJobThreadType::MainThread)
    {
        COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatEnqueueToMain"));

        if (threadData)
        {
            mainThreadJobs[priority].enqueue(coro.address(), threadData->mainEnqToken[priority]);
        }
        else
        {
            mainThreadJobs[priority].enqueue(coro.address());
        }
    }
    else if (enqueueToThread == EJobThreadType::WorkerThreads)
    {
        COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatEnqueueToWorker"));
        COPAT_ASSERT(!workersFinishedEvent.try_wait());

        if (threadData)
        {
            workerJobs[priority].enqueue(coro.address(), threadData->workerEnqDqToken[priority]);
        }
        else
        {
            workerJobs[priority].enqueue(coro.address());
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
            // Special thread queue token must not be null in this case
            COPAT_ASSERT(threadData->specialThreadTokens);
            specialThreadsPool.enqueueJob(coro, enqueueToThread, priority, threadData->specialThreadTokens);
        }
        else
        {
            specialThreadsPool.enqueueJob(coro, enqueueToThread, priority, nullptr);
        }
    }
}

JobSystem::PerThreadData::PerThreadData(
    WorkerThreadQueueType *workerQs, SpecialThreadQueueType *mainQs, SpecialThreadsPoolType &specialThreadPool
)
    : threadType(EJobThreadType::WorkerThreads)
    , workerEnqDqToken{ workerQs[Priority_Critical].getHazardToken(), workerQs[Priority_Normal].getHazardToken(),
                        workerQs[Priority_Low].getHazardToken() }
    , mainEnqToken{ mainQs[Priority_Critical].getHazardToken(), mainQs[Priority_Normal].getHazardToken(),
                    mainQs[Priority_Low].getHazardToken() }
    , specialThreadTokens(specialThreadPool.allocateEnqTokens())
{}

copat::JobSystem::PerThreadData &JobSystem::getOrCreatePerThreadData() noexcept
{
    PerThreadData *threadData = (PerThreadData *)PlatformThreadingFuncs::getTlsSlotValue(tlsSlot);
    if (!threadData)
    {
        PerThreadData *newThreadData = memNew<PerThreadData>(workerJobs, mainThreadJobs, specialThreadsPool);

        PlatformThreadingFuncs::setTlsSlotValue(tlsSlot, newThreadData);
        threadData = (PerThreadData *)PlatformThreadingFuncs::getTlsSlotValue(tlsSlot);
    }
    return *threadData;
}

void JobSystem::runMain() noexcept
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

        // Execute all tasks in Higher priority to lower priority order
        void *coroPtr = nullptr;
        for (EJobPriority priority = Priority_Critical; priority < Priority_MaxPriority && coroPtr == nullptr;
             priority = EJobPriority(priority + 1))
        {
            coroPtr = mainThreadJobs[priority].dequeue();
        }
        while (coroPtr)
        {
            COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatMainJob"));
            std::coroutine_handle<>::from_address(coroPtr).resume();

            coroPtr = nullptr;
            for (EJobPriority priority = Priority_Critical; priority < Priority_MaxPriority && coroPtr == nullptr;
                 priority = EJobPriority(priority + 1))
            {
                coroPtr = mainThreadJobs[priority].dequeue();
            }
        }

        if (bExitMain[0].test(std::memory_order::relaxed))
        {
            break;
        }
    }
}

void JobSystem::doWorkerJobs() noexcept
{
    PerThreadData *tlData = &getOrCreatePerThreadData();
    tlData->threadType = EJobThreadType::WorkerThreads;
    while (true)
    {
        // Execute all tasks in Higher priority to lower priority order
        void *coroPtr = nullptr;
        for (EJobPriority priority = Priority_Critical; priority < Priority_MaxPriority && coroPtr == nullptr;
             priority = EJobPriority(priority + 1))
        {
            coroPtr = workerJobs[priority].dequeue();
        }
        while (coroPtr)
        {
            COPAT_PROFILER_SCOPE(COPAT_PROFILER_CHAR("CopatWorkerJob"));
            std::coroutine_handle<>::from_address(coroPtr).resume();

            coroPtr = nullptr;
            for (EJobPriority priority = Priority_Critical; priority < Priority_MaxPriority && coroPtr == nullptr;
                 priority = EJobPriority(priority + 1))
            {
                coroPtr = workerJobs[priority].dequeue();
            }
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

copat::u32 JobSystem::calculateWorkersCount() const noexcept
{
    u32 coreCount, logicalProcCount;
    getCoreCount(coreCount, logicalProcCount);
    coreCount = coreCount > 4 ? coreCount : 4;
    return coreCount > MAX_SUPPORTED_WORKERS ? MAX_SUPPORTED_WORKERS : coreCount;
}

copat::JobSystem::PerThreadData *JobSystem::getPerThreadData() const noexcept
{
    return (PerThreadData *)PlatformThreadingFuncs::getTlsSlotValue(tlsSlot);
}

void INTERNAL_initializeAndRunSpecialThread(
    INTERNAL_SpecialThreadFuncType threadFunc, EJobThreadType threadType, u32 threadIdx, JobSystem *jobSystem
)
{
    u32 coreCount, logicalProcCount;
    getCoreCount(coreCount, logicalProcCount);

    std::thread specialThread{ [jobSystem, threadFunc]()
                               {
                                   (jobSystem->*threadFunc)();
                               } };
    PlatformThreadingFuncs::setThreadName(JobSystem::SpecialThreadsPoolType::NAMES[threadIdx], specialThread.native_handle());
    if (coreCount > u32(threadType))
    {
        // If not enough core just run as free thread
        PlatformThreadingFuncs::setThreadProcessor(u32(threadType), 0, specialThread.native_handle());
    }
    // Destroy when finishes
    specialThread.detach();
}

} // namespace copat