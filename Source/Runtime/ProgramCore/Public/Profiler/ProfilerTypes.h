/*!
 * \file ProfilerTypes.h
 *
 * \author Jeslas
 * \date April 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"

#ifdef USE_TRACY_PROFILER

using CBEProfilerChar = AChar;
#define CBE_PROFILER_CHAR(str) str
#define WCHAR_TO_PROFILERCHAR(WCharPtr) TCHAR_TO_UTF8(WCHAR_TO_TCHAR(WCharPtr))
#define ACHAR_TO_PROFILERCHAR(ACharPtr) ACharPtr
#define PROFILERCHAR_TO_TCHAR(CharPtr) UTF8_TO_TCHAR(CharPtr)

#define CBE_PROFILER_STRLITERAL AStringLiteral

#define CBE_PROFILER_COLOR(Colour) (Colour).bgra()

// Copied directly from ___tracy_source_location_data in TracyC.h
struct TracySrcLocDataType
{
    const CBEProfilerChar *name;
    const CBEProfilerChar *function;
    const CBEProfilerChar *file;
    uint32 line;
    uint32 color;
};
// Copied directly from ___tracy_c_zone_context in TracyC.h
struct TracyCZoneCtxType
{
    uint32 id;
    int32 active;
};
using CBEProfilerZoneCtx = TracyCZoneCtxType;
using CBEProfilerSrcLoc = TracySrcLocDataType;
using CBEProfilerTransientSrcLoc = uint64;

#else

using CBEProfilerZoneCtx = const void *;
using CBEProfilerSrcLoc = NullType;
using CBEProfilerTransientSrcLoc = CBEProfilerSrcLoc *;
using CBEProfilerChar = TChar;
#define CBE_PROFILER_CHAR(str) TCHAR(str)
#define WCHAR_TO_PROFILERCHAR(WCharPtr) WCHAR_TO_TCHAR(WCharPtr)
#define ACHAR_TO_PROFILERCHAR(ACharPtr) UTF8_TO_TCHAR(ACharPtr)
#define PROFILERCHAR_TO_TCHAR(CharPtr) CharPtr

#define CBE_PROFILER_STRLITERAL StringLiteral

#define CBE_PROFILER_COLOR(Colour) Colour

#endif