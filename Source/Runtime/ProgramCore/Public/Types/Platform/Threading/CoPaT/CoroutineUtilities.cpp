/*!
 * \file CoroutineUtilities.cpp
 *
 * \author Jeslas
 * \date April 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
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

} // namespace copat