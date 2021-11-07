#pragma once


#if PLATFORM_WINDOWS

#include "WindowsPlatformDefines.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif
