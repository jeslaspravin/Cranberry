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

JobSystem *JobSystem::singletonInstance = nullptr;

void JobSystem::initialize(MainThreadTickFunc &&mainTick, void *inUserData)
{
    if (!singletonInstance)
        singletonInstance = this;

    if (!PlatformThreadingFuncs::createTlsSlot(tlsSlot))
    {
        return;
    }

    specialThreadsPool.initialize(this);

    for (u32 i = 0; i < workersCount; ++i)
    {
        std::thread worker{ [this]() { doWorkerJobs(); } };
        PlatformThreadingFuncs::setThreadName((COPAT_TCHAR("WorkerThread_") + COPAT_TOSTRING(i)).c_str(), worker.native_handle());
        // Destroy when finishes
        worker.detach();
    }

    // Setup main thread
    mainThreadTick = std::forward<MainThreadTickFunc>(mainTick);
    userData = inUserData;
    PlatformThreadingFuncs::setCurrentThreadName(COPAT_TCHAR("MainThread"));
    PerThreadData &mainThreadData = getOrCreatePerThreadData();
    mainThreadData.threadType = EJobThreadType::MainThread;
}

void INTERNAL_initializeAndRunSpecialThread(INTERNAL_SpecialThreadFuncType threadFunc, EJobThreadType threadType, JobSystem *jobSystem)
{
    std::thread specialThread{ [jobSystem, threadFunc]() { (jobSystem->*threadFunc)(); } };
    PlatformThreadingFuncs::setThreadName(
        (COPAT_TCHAR("SpecialThread_") + COPAT_TOSTRING(u32(threadType))).c_str(), specialThread.native_handle()
    );
    // Destroy when finishes
    specialThread.detach();
}

} // namespace copat