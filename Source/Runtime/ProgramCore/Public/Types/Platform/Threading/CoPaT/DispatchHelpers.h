/*!
 * \file DispatchHelpers.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoPaTConfig.h"
#include "CoPaTTypes.h"

COPAT_NS_INLINED
namespace copat
{
class JobSystem;
class JobSystemPromiseBase;

template <typename RetType, typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread>
class JobSystemTaskType;

template <typename AwaitingCollection>
class AwaitAllTasks;

using DispatchAwaitableType = JobSystemTaskType<void, JobSystemPromiseBase, true, EJobThreadType::WorkerThreads>;
using DispatchFunctionType = FunctionType<void, u32>;

COPAT_EXPORT_SYM AwaitAllTasks<std::vector<DispatchAwaitableType>> dispatch(JobSystem *jobSys, const DispatchFunctionType &callback, u32 count);

} // namespace copat
