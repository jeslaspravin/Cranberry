/*!
 * \file DispatchHelpers.cpp
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "DispatchHelpers.h"
#include "CoroutineAwaitAll.h"
#include "JobSystemCoroutine.h"
#include "JobSystem.h"

COPAT_NS_INLINED
namespace copat
{
DispatchAwaitableType dispatchOneTask(JobSystem &jobSys, const DispatchFunctionType &callback, u32 jobIdx)
{
    callback(jobIdx);
    co_return;
}
DispatchAwaitableType dispatchTaskGroup(JobSystem &jobSys, const DispatchFunctionType &callback, u32 fromJobIdx, u32 count)
{
    const u32 endJobIdx = fromJobIdx + count;
    for (u32 jobIdx = fromJobIdx; jobIdx < endJobIdx; ++jobIdx)
    {
        callback(jobIdx);
    }
    co_return;
}

AwaitAllTasks<std::vector<DispatchAwaitableType>> dispatch(JobSystem *jobSys, const DispatchFunctionType &callback, u32 count)
{
    if (count == 0)
    {
        return {};
    }

    if (!jobSys)
    {
        // No job system just call all functions serially
        for (u32 i = 0; i < count; ++i)
        {
            callback(i);
        }
        return awaitAllTasks(std::vector<DispatchAwaitableType>{});
    }

    std::vector<DispatchAwaitableType> dispatchedJobs;

    u32 groupJobsCount = count / jobSys->getWorkersCount();
    // If dispatching count is less than max workers count
    if (groupJobsCount == 0)
    {
        dispatchedJobs.reserve(count);
        for (u32 i = 0; i < count; ++i)
        {
            dispatchedJobs.emplace_back(std::move(dispatchOneTask(*jobSys, callback, i)));
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
            dispatchedJobs.emplace_back(std::move(dispatchTaskGroup(*jobSys, callback, jobIdx, groupJobsCount + 1)));
            jobIdx += groupJobsCount + 1;
        }

        for (u32 i = grpsWithMoreJobCount; i < jobSys->getWorkersCount(); ++i)
        {
            dispatchedJobs.emplace_back(std::move(dispatchTaskGroup(*jobSys, callback, jobIdx, groupJobsCount)));
            jobIdx += groupJobsCount;
        }
    }
    return awaitAllTasks(std::move(dispatchedJobs));
}

} // namespace copat
