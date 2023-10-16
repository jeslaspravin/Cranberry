/*!
 * \file ConsoleMain.cpp
 *
 * \author Subity
 * \date September 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CmdLine/CmdLine.h"
#include "Logger/Logger.h"
#include "Memory/Memory.h"
#include "Modules/ModuleManager.h"
#include "Types/CoreTypes.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"
#include "Types/Platform/Threading/CoPaT/CoroutineAwaitAll.h"
#include "Types/Platform/Threading/CoPaT/CoroutineWait.h"
#include "Types/Platform/Threading/CoPaT/DispatchHelpers.h"
#include "Types/Platform/Threading/CoPaT/CoroutineUtilities.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Time.h"

#include <iostream>

// Override new and delete
CBE_GLOBAL_NEWDELETE_OVERRIDES

bool bQuit = false;

copat::JobSystemWorkerThreadTask enqForevStub(copat::EJobPriority jobPriority, uint32 idx)
{
    char priorityC[] = { 'L', 'N', 'C' };
    LOG("TestConsole", "Idx {}, Priority {}", idx, priorityC[jobPriority]);

    co_return;
}

copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread, copat::EJobPriority::Priority_Normal>
execInRenderingThread(LambdaFunction<void, uint32> execFunc)
{
    execFunc(45);

    co_return;
}

copat::NormalFuncAwaiter enqueueForever(uint32 count)
{
    std::vector<copat::JobSystemWorkerThreadTask> tasks;

    for (uint32 i = 0; i < count; ++i)
    {
        tasks.emplace_back(enqForevStub(copat::Priority_Low, i));
    }
    for (uint32 i = 0; i < count; ++i)
    {
        tasks.emplace_back(enqForevStub(copat::Priority_Normal, i));
    }
    for (uint32 i = 0; i < count; ++i)
    {
        tasks.emplace_back(enqForevStub(copat::Priority_Critical, i));
    }

    int32 largeArr[4096] = { 0 };
    auto lamb = [=](uint32 jobIdx) mutable
    {
        largeArr[jobIdx] += 10;
    };
    auto dispatches = copat::dispatch(copat::JobSystem::get(), copat::DispatchFunctionType::createLambda(lamb), 4);

    std::vector<copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread, copat::EJobPriority::Priority_Normal>> rendertasks;
    for (uint32 i = 0; i < count; ++i)
    {
        rendertasks.emplace_back(execInRenderingThread(lamb));
    }

    co_await copat::awaitAllTasks(copat::awaitAllTasks(std::move(tasks)), std::move(dispatches), copat::awaitAllTasks(std::move(rendertasks)));

    LOG("TestConsole", "Total Allocs {}, Total Reuses {}, Active {}, In delete queue {}, Deleted {}",
        copat::getNodeAllocsTracker().newAllocsCount.load(), copat::getNodeAllocsTracker().reuseCount.load(),
        copat::getNodeAllocsTracker().activeAllocs.load(), copat::getNodeAllocsTracker().inDeleteQAllocs.load(),
        copat::getNodeAllocsTracker().deletedCount.load());

    if (bQuit)
    {
        co_return;
    }

    Logger::flushStream();
    enqueueForever(count);
}

void doMain(void *)
{
    enqueueForever(2048);

    char c;
    std::cin >> c;
    if (c == 'q')
    {
        bQuit = true;
        copat::JobSystem::get()->exitMain();
    }
    Logger::flushStream();
}

int32 main(int32 argsc, AChar **args)
{
    UnexpectedErrorHandler::getHandler()->registerFilter();

    ModuleManager *moduleManager = ModuleManager::get();
    moduleManager->loadModule(TCHAR("ProgramCore"));
    // initializeCmdArguments();

    if (!ProgramCmdLine::get().parse(args, argsc))
    {
        // We cannot initialize logger before parsing command line args
        Logger::initialize();
        LOG_ERROR("TestConsole", "Failed to parse command line arguments");
        ProgramCmdLine::get().printCommandLine();
    }
    Logger::initialize();
    if (ProgramCmdLine::get().printHelp())
    {
        // Since this invocation is for printing help
        return 0;
    }

    Logger::flushStream();
    Logger::startLoggingTime();

    int32 returnCode = 0;
    {
        CBE_START_PROFILER();
        copat::JobSystem js{ copat::JobSystem::NoConstraints };
        js.initialize(copat::JobSystem::MainThreadTickFunc::createStatic(&doMain), nullptr);

        js.joinMain();

        js.shutdown();

        LOG("TestConsole", "Total Allocs {}, Total Reuses {}, Deleted {}", copat::getNodeAllocsTracker().newAllocsCount.load(),
            copat::getNodeAllocsTracker().reuseCount.load(), copat::getNodeAllocsTracker().deletedCount.load());
        CBE_STOP_PROFILER();
    }

    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    Logger::stopLoggingTime();
    Logger::shutdown();
    return returnCode;
}