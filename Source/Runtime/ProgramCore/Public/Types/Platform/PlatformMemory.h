/*!
 * \file PlatformMemory.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS

#include "WindowsPlatformMemory.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif

using PlatformMemory = GPlatformMemory::PlatformMemory;