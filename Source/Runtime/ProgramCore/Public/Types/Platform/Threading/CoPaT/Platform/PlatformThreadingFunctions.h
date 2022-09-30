#pragma once

#include "../CoPaTConfig.h"

#ifndef OVERRIDE_PLATFORMTHREADINGFUNCTIONS

#ifdef _WIN32
#include "WindowsThreadingFunctions.h"
COPAT_NS_INLINED
namespace copat
{
using PlatformThreadingFuncs = WindowsThreadingFunctions;
}
#elif __unix__
#error Not supported platform
#elif __linux__
#error Not supported platform
#elif __APPLE__
#error Not supported platform
#endif

#else
COPAT_NS_INLINED
namespace copat
{
using PlatformThreadingFuncs = OVERRIDE_PLATFORMTHREADINGFUNCTIONS;
} // namespace copat
#endif