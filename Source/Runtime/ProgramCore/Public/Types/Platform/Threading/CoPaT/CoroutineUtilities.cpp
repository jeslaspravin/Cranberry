/*!
 * \file CoroutineUtilities.cpp
 *
 * \author Jeslas
 * \date April 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoroutineUtilities.h"
#include "JobSystem.h"

COPAT_NS_INLINED
namespace copat
{
JobSystemFuncAwaiter::PromiseType::PromiseType()
    : enqToJobSystem(JobSystem::get())
{}

void SwitchJobSystemThreadAwaiter::enqueueToJs(std::coroutine_handle<> h, EJobPriority priority) const noexcept
{
    COPAT_ASSERT(switchToJs);
    switchToJs->enqueueJob(h, switchToThread, priority);
}

} // namespace copat