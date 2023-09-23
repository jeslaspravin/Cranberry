/*!
 * \file PlatformInstances.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS

#include "WindowsAppInstance.h"
#include "WindowsAppWindow.h"

#elif PLATFORM_LINUX
#error "Platform not supported!"
#elif PLATFORM_APPLE
#error "Platform not supported!"
#endif

typedef GPlatformInstances::PlatformAppInstance PlatformAppInstance;
typedef GPlatformInstances::PlatformAppWindow PlatformAppWindow;
