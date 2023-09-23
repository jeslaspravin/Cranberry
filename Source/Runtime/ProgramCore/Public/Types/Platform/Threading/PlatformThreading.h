/*!
 * \file PlatformThreading.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS

#include "Threading/WindowsThreadingFunctions.h"

#elif PLATFORM_LINUX
#error "Platform not supported!"
#elif PLATFORM_APPLE
#error "Platform not supported!"
#endif

using PlatformThreadingFunctions = GPlatformThreadingFunctions::PlatformThreadingFunctions;