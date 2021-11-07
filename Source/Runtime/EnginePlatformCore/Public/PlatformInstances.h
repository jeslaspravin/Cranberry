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
