/*!
 * \file ProgramProfiler.cpp
 *
 * \author Jeslas
 * \date January 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Profiler/ProgramProfiler.hpp"
#include "Memory/Memory.h"
#include "String/TCharString.h"

static std::atomic_flag bIsProfilerRunning;

#ifdef USE_TRACY_PROFILER

#define TRACY_CALLSTACK 60
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

static_assert(
    sizeof(TracyCZoneCtx) == sizeof(CBEProfilerZoneCtx) && alignof(TracyCZoneCtx) <= alignof(CBEProfilerZoneCtx),
    "Zone Context mismatch between Tracy and CBEProfiler"
);
static_assert(
    sizeof(___tracy_source_location_data) == sizeof(CBEProfilerSrcLoc) && alignof(___tracy_source_location_data) <= alignof(CBEProfilerSrcLoc),
    "Source location data mismatch between Tracy and CBEProfiler"
);

#define CBE_START_PROFILER_internal() tracy::StartupProfiler()
#define CBE_STOP_PROFILER_internal() tracy::ShutdownProfiler()

#define CBE_PROFILER_AVAILABLE() tracy::ProfilerAvailable()

#define CBE_PROFILER_MARKFRAME_internal() FrameMark
#define CBE_PROFILER_MARKFRAME_N_internal(Text) FrameMarkNamed(Text)
#define CBE_PROFILER_STARTFRAME_internal(Text) FrameMarkStart(Text)
#define CBE_PROFILER_ENDFRAME_internal(Text) FrameMarkEnd(Text)

#define CBE_PROFILER_MESSAGE_internal(Text) TracyMessage(Text, TCharStr::length(Text))
#define CBE_PROFILER_MESSAGE_L_internal(Text) TracyMessageL(Text)
#define CBE_PROFILER_MESSAGE_C_internal(Text, Colour) TracyMessageC(Text, TCharStr::length(Text), CBE_PROFILER_COLOR(Colour))
#define CBE_PROFILER_MESSAGE_LC_internal(Text, Colour) TracyMessageLC(Text, CBE_PROFILER_COLOR(Colour))

#define CBE_PROFILER_ALLOC_internal(ptr, size) TracySecureAlloc(ptr, size)
#define CBE_PROFILER_ALLOC_N_internal(ptr, size, name) TracySecureAllocN(ptr, size, name)
#define CBE_PROFILER_FREE_internal(ptr) TracySecureFree(ptr)
#define CBE_PROFILER_FREE_N_internal(ptr, name) TracySecureFreeN(ptr, name)

#define CBE_PROFILER_ENTERFIBER_internal(Text) TracyFiberEnter(Text)
#define CBE_PROFILER_LEAVEFIBER_internal() TracyFiberLeave

#define CBE_PROFILER_ALLOCATE_SRC_LOC(FuncName, FileName, Line, Colour)                                                                        \
    ___tracy_alloc_srcloc(Line, FileName, TCharStr::length(FileName), FuncName, TCharStr::length(FuncName))
#define CBE_PROFILER_ALLOCATE_SRC_LOC_N(Name, FuncName, FileName, Line, Colour)                                                                \
    ___tracy_alloc_srcloc_name(Line, FileName, TCharStr::length(FileName), FuncName, TCharStr::length(FuncName), Name, TCharStr::length(Name))

#define TRACY_SCOPE_CTX_TO_CBEPROFILER_CTX(Ctx)                                                                                                \
    [](auto ctx)                                                                                                                               \
    {                                                                                                                                          \
        return *reinterpret_cast<CBEProfilerZoneCtx *>(&ctx);                                                                                  \
    }(Ctx)

#if defined TRACY_HAS_CALLSTACK && defined TRACY_CALLSTACK

#define CBE_PROFILER_BEGIN_STATIC_SCOPE(SrcLoc, Active)                                                                                        \
    TRACY_SCOPE_CTX_TO_CBEPROFILER_CTX(                                                                                                        \
        ___tracy_emit_zone_begin_callstack(reinterpret_cast<const ___tracy_source_location_data *>(SrcLoc), TRACY_CALLSTACK, Active)           \
    )
#define CBE_PROFILER_BEGIN_TRANSIENT_SCOPE(SrcLoc, Active)                                                                                     \
    TRACY_SCOPE_CTX_TO_CBEPROFILER_CTX(___tracy_emit_zone_begin_alloc_callstack(SrcLoc, TRACY_CALLSTACK, Active))

#else // defined TRACY_HAS_CALLSTACK && defined TRACY_CALLSTACK

#define CBE_PROFILER_BEGIN_STATIC_SCOPE(SrcLoc, Active)                                                                                        \
    TRACY_SCOPE_CTX_TO_CBEPROFILER_CTX(___tracy_emit_zone_begin(reinterpret_cast<const ___tracy_source_location_data *>(SrcLoc), Active))
#define CBE_PROFILER_BEGIN_TRANSIENT_SCOPE(SrcLoc, Active) TRACY_SCOPE_CTX_TO_CBEPROFILER_CTX(___tracy_emit_zone_begin_alloc(SrcLoc, Active))

#endif // defined TRACY_HAS_CALLSTACK && defined TRACY_CALLSTACK
#define CBE_PROFILER_END_SCOPE(Ctx) TracyCZoneEnd(*reinterpret_cast<const TracyCZoneCtx *>(&Ctx))

#define CBE_PROFILER_SCOPE_SETTEXT(Ctx, Text) TracyCZoneText(*reinterpret_cast<const TracyCZoneCtx *>(&Ctx), Text, TCharStr::length(Text))
#define CBE_PROFILER_SCOPE_SETNAME(Ctx, Text) TracyCZoneName(*reinterpret_cast<const TracyCZoneCtx *>(&Ctx), Text, TCharStr::length(Text))
#define CBE_PROFILER_SCOPE_SETCOLOR(Ctx, Colour) TracyCZoneColor(*reinterpret_cast<const TracyCZoneCtx *>(&Ctx), CBE_PROFILER_COLOR(Colour))
#define CBE_PROFILER_SCOPE_SETVALUE(Ctx, Value) TracyCZoneValue(*reinterpret_cast<const TracyCZoneCtx *>(&Ctx), Value)

#else

#define CBE_START_PROFILER_internal()
#define CBE_STOP_PROFILER_internal()

#define CBE_PROFILER_AVAILABLE() false

#define CBE_PROFILER_MARKFRAME_internal()
#define CBE_PROFILER_MARKFRAME_N_internal(Text)
#define CBE_PROFILER_STARTFRAME_internal(Text)
#define CBE_PROFILER_ENDFRAME_internal(Text)

#define CBE_PROFILER_MESSAGE_internal(Text)
#define CBE_PROFILER_MESSAGE_L_internal(Text)
#define CBE_PROFILER_MESSAGE_C_internal(Text, Colour)
#define CBE_PROFILER_MESSAGE_LC_internal(Text, Colour)

#define CBE_PROFILER_ALLOC_internal(ptr, size)
#define CBE_PROFILER_ALLOC_N_internal(ptr, size, name)
#define CBE_PROFILER_FREE_internal(ptr, size)
#define CBE_PROFILER_FREE_N_internal(ptr, size, name)

#define CBE_PROFILER_ENTERFIBER_internal(Text)
#define CBE_PROFILER_LEAVEFIBER_internal()

#define CBE_PROFILER_ALLOCATE_SRC_LOC(FuncName, FileName, Line, Colour)
#define CBE_PROFILER_ALLOCATE_SRC_LOC_N(Name, FuncName, FileName, Line, Colour)

#define CBE_PROFILER_BEGIN_STATIC_SCOPE(SrcLoc, Active)
#define CBE_PROFILER_BEGIN_TRANSIENT_SCOPE(SrcLoc, Active)
#define CBE_PROFILER_END_SCOPE(Ctx)

#define CBE_PROFILER_SCOPE_SETTEXT(Ctx, Text)
#define CBE_PROFILER_SCOPE_SETNAME(Ctx, Text)
#define CBE_PROFILER_SCOPE_SETCOLOR(Ctx, Colour)
#define CBE_PROFILER_SCOPE_SETVALUE(Ctx, Value)

#endif

void CBEProfiler::startProfiler()
{
    CBE_START_PROFILER_internal();
    bIsProfilerRunning.test_and_set();
}
void CBEProfiler::stopProfiler()
{
    bIsProfilerRunning.clear();
    CBE_STOP_PROFILER_internal();
}

bool CBEProfiler::profilerAvailable() { return CBE_PROFILER_AVAILABLE(); }

void CBEProfiler::markFrame()
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_MARKFRAME_internal();
    }
}
void CBEProfiler::markFrame(const CBEProfilerChar *text)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_MARKFRAME_N_internal(text);
    }
}
void CBEProfiler::startFrame(const CBEProfilerChar *text)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_STARTFRAME_internal(text);
    }
}
void CBEProfiler::endFrame(const CBEProfilerChar *text)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_ENDFRAME_internal(text);
    }
}

void CBEProfiler::sendMessageL(const CBEProfilerChar *text)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_MESSAGE_L_internal(text);
    }
}
void CBEProfiler::sendMessageLC(const CBEProfilerChar *text, Color color)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_MESSAGE_LC_internal(text, color);
    }
}

void CBEProfiler::enterFiber(const CBEProfilerChar *name)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif // SECURE_PROFILING
    {
        CBE_PROFILER_ENTERFIBER_internal(name);
    }
}

void CBEProfiler::leaveFiber()
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif // SECURE_PROFILING
    {
        CBE_PROFILER_LEAVEFIBER_internal();
    }
}

#ifdef ENABLE_MEMORY_PROFILING
void CBEProfiler::trackAlloc(void *ptr, SizeT size, const CBEProfilerChar *name)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif // SECURE_PROFILING
    {
        CBE_PROFILER_ALLOC_N_internal(ptr, size, name);
    }
}
void CBEProfiler::trackAlloc(void *ptr, SizeT size)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif // SECURE_PROFILING
    {
        CBE_PROFILER_ALLOC_internal(ptr, size);
    }
}
void CBEProfiler::trackFree(void *ptr, const CBEProfilerChar *name)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif // SECURE_PROFILING
    {
        CBE_PROFILER_FREE_N_internal(ptr, name);
    }
}
void CBEProfiler::trackFree(void *ptr)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif // SECURE_PROFILING
    {
        CBE_PROFILER_FREE_internal(ptr);
    }
}

#else  // ENABLE_MEMORY_PROFILING
void CBEProfiler::trackAlloc(void *, SizeT, const CBEProfilerChar *) {}
void CBEProfiler::trackAlloc(void *, SizeT) {}
void CBEProfiler::trackFree(void *, const CBEProfilerChar *) {}
void CBEProfiler::trackFree(void *) {}
#endif // ENABLE_MEMORY_PROFILING

void CBEProfiler::sendMessagePrivate(const CBEProfilerChar *text)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_MESSAGE_internal(text);
    }
}
void CBEProfiler::sendMessageCPrivate(const CBEProfilerChar *text, Color color)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_MESSAGE_C_internal(text, color);
    }
}

CBEProfilerZoneCtx CBEProfiler::beginStaticScope(const CBEProfilerSrcLoc *srcLoc, bool bActive)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#else
    if constexpr (true)
#endif
    {
        return CBE_PROFILER_BEGIN_STATIC_SCOPE(srcLoc, bActive);
    }
    else
    {
        return {};
    }
}
void CBEProfiler::endStaticScope(CBEProfilerZoneCtx ctx)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_END_SCOPE(ctx);
    }
}

CBEProfilerTransientSrcLoc CBEProfiler::allocateTransientSrcLocPrivate(
    const CBEProfilerChar *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color
)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#else
    if constexpr (true)
#endif
    {
        CompilerHacks::ignoreUnused(color);
        return CBE_PROFILER_ALLOCATE_SRC_LOC_N(name, function, file, line, color);
    }
    else
    {
        return {};
    }
}
CBEProfilerZoneCtx CBEProfiler::beginTransientScope(const CBEProfilerTransientSrcLoc srcLoc, bool bActive)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#else
    if constexpr (true)
#endif
    {
        return CBE_PROFILER_BEGIN_TRANSIENT_SCOPE(srcLoc, bActive);
    }
    else
    {
        return {};
    }
}
void CBEProfiler::endTransientScope(CBEProfilerZoneCtx ctx)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_END_SCOPE(ctx);
    }
}

void CBEProfiler::setScopeColor(CBEProfilerZoneCtx ctx, Color color)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_SCOPE_SETCOLOR(ctx, color);
    }
}
void CBEProfiler::setScopeValue(CBEProfilerZoneCtx ctx, uint64 value)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_SCOPE_SETVALUE(ctx, value);
    }
}
void CBEProfiler::setScopeTextPrivate(CBEProfilerZoneCtx ctx, const CBEProfilerChar *text)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_SCOPE_SETTEXT(ctx, text);
    }
}
void CBEProfiler::setScopeNamePrivate(CBEProfilerZoneCtx ctx, const CBEProfilerChar *name)
{
#ifdef SECURE_PROFILING
    if (profilerAvailable())
#endif
    {
        CBE_PROFILER_SCOPE_SETNAME(ctx, name);
    }
}
