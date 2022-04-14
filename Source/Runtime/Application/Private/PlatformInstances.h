/*!
 * \file PlatformInstances.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS

#include "WindowsAppInstance.h"
#include "WindowsAppWindow.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif

typedef GPlatformInstances::PlatformAppInstance PlatformAppInstance;
typedef GPlatformInstances::PlatformAppWindow PlatformAppWindow;
