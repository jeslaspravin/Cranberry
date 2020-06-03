#pragma once


#if _WIN32

#include "Windows/WindowsPlatformDefines.h"

#elif __unix__
static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif
