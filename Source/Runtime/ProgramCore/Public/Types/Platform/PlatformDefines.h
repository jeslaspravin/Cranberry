/*!
 * \file PlatformDefines.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 0
#endif

#ifndef PLATFORM_LINUX
#define PLATFORM_LINUX 0
#endif

#ifndef PLATFORM_APPLE
#define PLATFORM_APPLE 0
#endif

#ifndef PLATFORM_64
#define PLATFORM_64 0
#endif

#ifndef PLATFORM_32
#define PLATFORM_32 0
#endif

#if PLATFORM_WINDOWS

#include "WindowsPlatformDefines.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif