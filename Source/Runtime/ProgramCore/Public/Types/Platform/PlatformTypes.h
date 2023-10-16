/*!
 * \file PlatformTypes.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS

#include "WindowsPlatformTypes.h"

#elif PLATFORM_LINUX
#error "Platform not supported!"
#elif PLATFORM_APPLE
#error "Platform not supported!"
#endif

using PlatformHandle = PlatformTypes::PlatformHandle;
using InstanceHandle = PlatformTypes::InstanceHandle;
using LibHandle = PlatformTypes::LibHandle;
using WindowHandle = PlatformTypes::WindowHandle;

using ProcAddress = PlatformTypes::ProcAddress;