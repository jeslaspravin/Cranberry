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

#include "JobSystem.h"

COPAT_NS_INLINED
namespace copat
{
class JobSystem;
class JobSystemPromiseBase;

template <typename RetType, typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
class JobSystemTaskType;

template <typename AwaitingCollection>
class AwaitAllTasks;

struct NormalFuncAwaiter;

template <typename RetType>
using DispatchAwaitableTypeWithRet
    = JobSystemTaskType<RetType, JobSystemPromiseBase, true, EJobThreadType::WorkerThreads, EJobPriority::Priority_Normal>;
template <typename RetType>
using DispatchFunctionTypeWithRet = FunctionType<RetType, u32>;

using DispatchAwaitableType = DispatchAwaitableTypeWithRet<void>;
using DispatchFunctionType = DispatchFunctionTypeWithRet<void>;

COPAT_EXPORT_SYM AwaitAllTasks<std::vector<DispatchAwaitableType>>
dispatch(JobSystem *jobSys, const DispatchFunctionType &callback, u32 count, EJobPriority jobPriority = EJobPriority::Priority_Normal) noexcept;

template <typename FuncType, typename... Args>
NormalFuncAwaiter fireAndForget(FuncType &&func, Args &&...args) noexcept
{
    FuncType funcCopy = std::forward<FuncType>(func);
    co_await funcCopy(std::forward<Args>(args)...);
}

template <typename RetType>
struct DispatchWithReturn
{
    using AwaitableType = DispatchAwaitableTypeWithRet<std::vector<RetType>>;
    using FuncType = DispatchFunctionTypeWithRet<RetType>;

    // Just copying the callback so a copy exists inside dispatch
    // Just copying the callback so a copy exists inside dispatch
    static AwaitableType dispatchOneTask(JobSystem &jobSys, EJobPriority jobPriority, FuncType callback, u32 jobIdx) noexcept
    {
        std::vector<RetType> retVal{ callback(jobIdx) };
        co_return retVal;
    }
    static AwaitableType dispatchTaskGroup(JobSystem &jobSys, EJobPriority jobPriority, FuncType callback, u32 fromJobIdx, u32 count) noexcept
    {
        std::vector<RetType> retVal;
        retVal.reserve(count);

        const u32 endJobIdx = fromJobIdx + count;
        for (u32 jobIdx = fromJobIdx; jobIdx < endJobIdx; ++jobIdx)
        {
            retVal.emplace_back(callback(jobIdx));
        }
        co_return retVal;
    }

    static AwaitAllTasks<std::vector<AwaitableType>>
    dispatch(JobSystem *jobSys, const FuncType &callback, u32 count, EJobPriority jobPriority) noexcept
    {
        if (count == 0)
        {
            return {};
        }

        // If there is no worker threads then we cannot dispatch so fail
        COPAT_ASSERT(jobSys && jobSys->enqToThreadType(EJobThreadType::WorkerThreads) == EJobThreadType::WorkerThreads);

        std::vector<AwaitableType> dispatchedJobs;

        u32 groupJobsCount = count / jobSys->getWorkersCount();
        // If dispatching count is less than max workers count
        if (groupJobsCount == 0)
        {
            dispatchedJobs.reserve(count);
            for (u32 i = 0; i < count; ++i)
            {
                dispatchedJobs.emplace_back(std::move(dispatchOneTask(*jobSys, jobPriority, callback, i)));
            }
        }
        else
        {
            dispatchedJobs.reserve(jobSys->getWorkersCount());
            u32 grpsWithMoreJobCount = count % jobSys->getWorkersCount();
            u32 jobIdx = 0;
            for (u32 i = 0; i < grpsWithMoreJobCount; ++i)
            {
                // Add one more job for all grps with more jobs
                dispatchedJobs.emplace_back(std::move(dispatchTaskGroup(*jobSys, jobPriority, callback, jobIdx, groupJobsCount + 1)));
                jobIdx += groupJobsCount + 1;
            }

            for (u32 i = grpsWithMoreJobCount; i < jobSys->getWorkersCount(); ++i)
            {
                dispatchedJobs.emplace_back(std::move(dispatchTaskGroup(*jobSys, jobPriority, callback, jobIdx, groupJobsCount)));
                jobIdx += groupJobsCount;
            }
        }
        return awaitAllTasks(std::move(dispatchedJobs));
    }
};

/**
 * This function assumes that worker threads exists and enqueue will work without any issue
 */
template <typename RetType>
auto diverge(
    JobSystem *jobSys, DispatchFunctionTypeWithRet<RetType> &&callback, u32 count, EJobPriority jobPriority = EJobPriority::Priority_Normal
) noexcept
{
    static_assert(
        std::is_same_v<DispatchFunctionTypeWithRet<RetType>, typename DispatchWithReturn<RetType>::FuncType>,
        "Type mismatch between dispatch and diverge functions"
    );
    return DispatchWithReturn<RetType>::dispatch(jobSys, std::forward<decltype(callback)>(callback), count, jobPriority);
}

template <typename RetType>
std::vector<RetType> converge(AwaitAllTasks<std::vector<DispatchAwaitableTypeWithRet<std::vector<RetType>>>> &&allAwaits)
{
    static_assert(
        std::is_same_v<DispatchAwaitableTypeWithRet<std::vector<RetType>>, typename DispatchWithReturn<RetType>::AwaitableType>,
        "Type mismatch between dispatch and diverge functions"
    );
    std::vector<RetType> retVal;
    for (auto &awaitable : waitOnAwaitable(allAwaits))
    {
        retVal.insert(retVal.end(), awaitable.getReturnValue().cbegin(), awaitable.getReturnValue().cend());
    }
    return retVal;
}

} // namespace copat
