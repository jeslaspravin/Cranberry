#pragma once

#if _WIN32

#include "Windows/WindowsAppInstance.h"
#include "Windows/WindowsAppWindow.h"

#elif __unix__

static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif


typedef GPlatformInstances::PlatformAppInstance PlatformAppInstance;
typedef GPlatformInstances::PlatformAppWindow PlatformAppWindow;
