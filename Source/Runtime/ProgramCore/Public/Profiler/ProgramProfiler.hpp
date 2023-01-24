/*!
 * \file ProgramProfiler.hpp
 *
 * \author Jeslas
 * \date January 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "String/String.h"
#include "Types/Colors.h"

#ifdef USE_TRACY_PROFILER

using CBEProfilerChar = AChar;
#define CBE_PROFILER_CHAR(str) str
#define WCHAR_TO_PROFILERCHAR(WCharPtr) TCHAR_TO_UTF8(WCHAR_TO_TCHAR(WCharPtr))
#define ACHAR_TO_PROFILERCHAR(ACharPtr) ACharPtr
#define PROFILERCHAR_TO_TCHAR(CharPtr) UTF8_TO_TCHAR(CharPtr)

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

#define CBE_PROFILER_COLOR(Colour) Colour

#endif

// Following Tracy's naming convention
// Literals must have program wide lifetime at least
// L - Literal, C - Color, S - With call stack
// N - Named
class PROGRAMCORE_EXPORT CBEProfiler
{
public:
    static void startProfiler();
    static void stopProfiler();

    NODISCARD static bool profilerAvailable();

    static void markFrame();
    static void markFrame(const CBEProfilerChar *text);
    static void startFrame(const CBEProfilerChar *text);
    static void endFrame(const CBEProfilerChar *text);

    static void sendMessage(const AChar *text) { sendMessagePrivate(ACHAR_TO_PROFILERCHAR(text)); }
    static void sendMessage(const WChar *text) { sendMessagePrivate(WCHAR_TO_PROFILERCHAR(text)); }
    static void sendMessageL(const CBEProfilerChar *text);
    static void sendMessageC(const AChar *text, Color color) { sendMessageCPrivate(ACHAR_TO_PROFILERCHAR(text), color); }
    static void sendMessageC(const WChar *text, Color color) { sendMessageCPrivate(WCHAR_TO_PROFILERCHAR(text), color); }
    static void sendMessageLC(const CBEProfilerChar *text, Color color);

    // Only literal names are allowed
    static void trackAlloc(void *ptr, SizeT size, const CBEProfilerChar *name);
    static void trackAlloc(void *ptr, SizeT size);
    static void trackFree(void *ptr, const CBEProfilerChar *name);
    static void trackFree(void *ptr);

    static void enterFiber(const CBEProfilerChar *name);
    static void leaveFiber();

    static CBEProfilerZoneCtx beginStaticScope(const CBEProfilerSrcLoc *srcLoc, bool bActive);
    static void endStaticScope(CBEProfilerZoneCtx ctx);

    FORCE_INLINE static CBEProfilerTransientSrcLoc
        allocateTransientSrcLoc(const AChar *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color)
    {
        return allocateTransientSrcLocPrivate(ACHAR_TO_PROFILERCHAR(name), function, file, line, color);
    }
    FORCE_INLINE static CBEProfilerTransientSrcLoc
        allocateTransientSrcLoc(const WChar *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color)
    {
        return allocateTransientSrcLocPrivate(WCHAR_TO_PROFILERCHAR(name), function, file, line, color);
    }
    static CBEProfilerZoneCtx beginTransientScope(const CBEProfilerTransientSrcLoc srcLoc, bool bActive);
    static void endTransientScope(CBEProfilerZoneCtx ctx);

    FORCE_INLINE static void setScopeText(CBEProfilerZoneCtx ctx, const WChar *text) { setScopeTextPrivate(ctx, WCHAR_TO_PROFILERCHAR(text)); }
    FORCE_INLINE static void setScopeText(CBEProfilerZoneCtx ctx, const AChar *text) { setScopeTextPrivate(ctx, ACHAR_TO_PROFILERCHAR(text)); }
    FORCE_INLINE static void setScopeName(CBEProfilerZoneCtx ctx, const WChar *name) { setScopeNamePrivate(ctx, WCHAR_TO_PROFILERCHAR(name)); }
    FORCE_INLINE static void setScopeName(CBEProfilerZoneCtx ctx, const AChar *name) { setScopeNamePrivate(ctx, ACHAR_TO_PROFILERCHAR(name)); }
    static void setScopeColor(CBEProfilerZoneCtx ctx, Color color);
    static void setScopeValue(CBEProfilerZoneCtx ctx, uint64 value);

private:
    CBEProfiler() = default;

    static void sendMessagePrivate(const CBEProfilerChar *text);
    static void sendMessageCPrivate(const CBEProfilerChar *text, Color color);

    static CBEProfilerTransientSrcLoc allocateTransientSrcLocPrivate(
        const CBEProfilerChar *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color
    );
    static void setScopeTextPrivate(CBEProfilerZoneCtx ctx, const CBEProfilerChar *text);
    static void setScopeNamePrivate(CBEProfilerZoneCtx ctx, const CBEProfilerChar *name);
};

struct CBEProfilerStaticScope
{
public:
    const CBEProfilerSrcLoc *srcLoc;
    CBEProfilerZoneCtx ctx;

    CBEProfilerStaticScope(const CBEProfilerSrcLoc &inSrcLoc, bool bActive)
        : srcLoc(&inSrcLoc)
        , ctx(CBEProfiler::beginStaticScope(srcLoc, bActive))
    {}
    CBEProfilerStaticScope(const CBEProfilerSrcLoc &inSrcLoc, bool bActive, uint64 value)
        : srcLoc(&inSrcLoc)
        , ctx(CBEProfiler::beginStaticScope(srcLoc, bActive))
    {
        CBEProfiler::setScopeValue(ctx, value);
    }
    template <typename CharType>
    CBEProfilerStaticScope(const CBEProfilerSrcLoc &inSrcLoc, bool bActive, const CharType *text)
        : srcLoc(&inSrcLoc)
        , ctx(CBEProfiler::beginStaticScope(srcLoc, bActive))
    {
        CBEProfiler::setScopeText(ctx, text);
    }
    template <typename CharType>
    CBEProfilerStaticScope(const CBEProfilerSrcLoc &inSrcLoc, bool bActive, const CharType *text, uint64 value)
        : srcLoc(&inSrcLoc)
        , ctx(CBEProfiler::beginStaticScope(srcLoc, bActive))
    {
        CBEProfiler::setScopeText(ctx, text);
        CBEProfiler::setScopeValue(ctx, value);
    }

    ~CBEProfilerStaticScope() { CBEProfiler::endStaticScope(ctx); }

    CBEProfilerStaticScope() = delete;
    MAKE_TYPE_NONCOPY_NONMOVE(CBEProfilerStaticScope)
};

struct CBEProfilerTransientScope
{
public:
    CBEProfilerTransientSrcLoc srcLoc;
    CBEProfilerZoneCtx ctx;

    template <typename CharType>
    CBEProfilerTransientScope(
        const CharType *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color, bool bActive
    )
        : srcLoc(CBEProfiler::allocateTransientSrcLoc(name, function, file, line, color))
        , ctx(CBEProfiler::beginTransientScope(&srcLoc, bActive))
    {
#if USE_TRACY_PROFILER
        CBEProfiler::setScopeColor(ctx, color);
#endif
    }
    template <typename CharType>
    CBEProfilerTransientScope(
        const CharType *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color, bool bActive, uint64 value
    )
        : srcLoc(CBEProfiler::allocateTransientSrcLoc(name, function, file, line, color))
        , ctx(CBEProfiler::beginTransientScope(&srcLoc, bActive))
    {
#if USE_TRACY_PROFILER
        CBEProfiler::setScopeColor(ctx, color);
#endif
        CBEProfiler::setScopeValue(ctx, value);
    }
    template <typename NameCharType, typename TextCharType>
    CBEProfilerTransientScope(
        const NameCharType *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color, bool bActive,
        const TextCharType *text
    )
        : srcLoc(CBEProfiler::allocateTransientSrcLoc(name, function, file, line, color))
        , ctx(CBEProfiler::beginTransientScope(&srcLoc, bActive))
    {
#if USE_TRACY_PROFILER
        CBEProfiler::setScopeColor(ctx, color);
#endif
        CBEProfiler::setScopeText(ctx, text);
    }
    template <typename NameCharType, typename TextCharType>
    CBEProfilerTransientScope(
        const NameCharType *name, const CBEProfilerChar *function, const CBEProfilerChar *file, uint32 line, Color color, bool bActive,
        const TextCharType *text, uint64 value
    )
        : srcLoc(CBEProfiler::allocateTransientSrcLoc(name, function, file, line, color))
        , ctx(CBEProfiler::beginTransientScope(&srcLoc, bActive))
    {
#if USE_TRACY_PROFILER
        CBEProfiler::setScopeColor(ctx, color);
#endif
        CBEProfiler::setScopeText(ctx, text);
        CBEProfiler::setScopeValue(ctx, value);
    }

    ~CBEProfilerTransientScope() { CBEProfiler::endTransientScope(ctx); }

    CBEProfilerTransientScope() = delete;
    MAKE_TYPE_NONCOPY_NONMOVE(CBEProfilerTransientScope)
};

#if ENABLE_PROFILING

#define CBE_START_PROFILER() CBEProfiler::startProfiler()
#define CBE_STOP_PROFILER() CBEProfiler::stopProfiler()

#define CBE_PROFILER_MARKFRAME() CBEProfiler::markFrame()
#define CBE_PROFILER_MARKFRAME_N(Text) CBEProfiler::markFrame(Text)
#define CBE_PROFILER_STARTFRAME(Text) CBEProfiler::startFrame(Text)
#define CBE_PROFILER_ENDFRAME(Text) CBEProfiler::endFrame(Text)

#define CBE_PROFILER_MESSAGE(Text) CBEProfiler::sendMessage(Text)
#define CBE_PROFILER_MESSAGE_L(Text) CBEProfiler::sendMessageL(Text)
#define CBE_PROFILER_MESSAGE_C(Text, Colour) CBEProfiler::sendMessageC(Text, Colour)
#define CBE_PROFILER_MESSAGE_LC(Text, Colour) CBEProfiler::sendMessageLC(Text, Colour)

#define CBE_PROFILER_ALLOC(Ptr, Size) CBEProfiler::trackAlloc(Ptr, Size)
#define CBE_PROFILER_ALLOC_N(Ptr, Size, Name) CBEProfiler::trackAlloc(Ptr, Size, Name)
#define CBE_PROFILER_FREE(Ptr) CBEProfiler::trackFree(Ptr)
#define CBE_PROFILER_FREE_N(Ptr, Name) CBEProfiler::trackFree(Ptr, Name)

#define CBE_PROFILER_ENTERFIBER(Text) CBEProfiler::enterFiber(Text)
#define CBE_PROFILER_LEAVEFIBER() CBEProfiler::leaveFiber()

#define zzzCBE_PROFILER_UNIQ_NAME(NamePrefix) COMBINE(NamePrefix, __LINE__)

// Following scoped macro's Name must have global lifetime, If name is temporary use TRANSIENT variants or use CBEProfiler::setScopeName
#define CBE_PROFILER_SCOPE_VAR_FULL(VarName, Name, ControlVar, Text, Colour, Value)                                                            \
    static const CBEProfilerSrcLoc zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_                                                                \
    ){ Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(Colour) };                                 \
    CBEProfilerStaticScope VarName                                                                                                             \
    {                                                                                                                                          \
        zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_), ControlVar, Text, Value                                                                 \
    }
#define CBE_PROFILER_SCOPE_VAR_TC(VarName, Name, ControlVar, Text, Colour)                                                                     \
    static const CBEProfilerSrcLoc zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_                                                                \
    ){ Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(Colour) };                                 \
    CBEProfilerStaticScope VarName                                                                                                             \
    {                                                                                                                                          \
        zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_), ControlVar, Text                                                                        \
    }
#define CBE_PROFILER_SCOPE_VAR_C(VarName, Name, ControlVar, Colour)                                                                            \
    static const CBEProfilerSrcLoc zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_                                                                \
    ){ Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(Colour) };                                 \
    CBEProfilerStaticScope VarName                                                                                                             \
    {                                                                                                                                          \
        zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_), ControlVar                                                                              \
    }
#define CBE_PROFILER_SCOPE_VAR(VarName, Name, ControlVar)                                                                                      \
    static const CBEProfilerSrcLoc zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_                                                                \
    ){ Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(ColorConst::BLACK_Transparent) };          \
    CBEProfilerStaticScope VarName                                                                                                             \
    {                                                                                                                                          \
        zzzCBE_PROFILER_UNIQ_NAME(cbeProfilerSrcLoc_), ControlVar                                                                              \
    }

// Below are Transient variant of scopes
#define CBE_PROFILER_TSCOPE_VAR_FULL(VarName, Name, ControlVar, Text, Colour, Value)                                                           \
    CBEProfilerTransientScope VarName                                                                                                          \
    {                                                                                                                                          \
        Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(Colour), ControlVar, Text, Value          \
    }
#define CBE_PROFILER_TSCOPE_VAR_TC(VarName, Name, ControlVar, Text, Colour)                                                                    \
    CBEProfilerTransientScope VarName                                                                                                          \
    {                                                                                                                                          \
        Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(Colour), ControlVar, Text                 \
    }
#define CBE_PROFILER_TSCOPE_VAR_C(VarName, Name, ControlVar, Colour)                                                                           \
    CBEProfilerTransientScope VarName                                                                                                          \
    {                                                                                                                                          \
        Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(Colour), ControlVar                       \
    }
#define CBE_PROFILER_TSCOPE_VAR(VarName, Name, ControlVar)                                                                                     \
    CBEProfilerTransientScope VarName                                                                                                          \
    {                                                                                                                                          \
        Name, CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__FILE__), __LINE__, CBE_PROFILER_COLOR(ColorConst::BLACK_Transparent),           \
            ControlVar                                                                                                                         \
    }

#else // ENABLE_PROFILING

#define CBE_START_PROFILER()
#define CBE_STOP_PROFILER()

#define CBE_PROFILER_MARKFRAME()
#define CBE_PROFILER_MARKFRAME_N(Text)
#define CBE_PROFILER_STARTFRAME(Text)
#define CBE_PROFILER_ENDFRAME(Text)

#define CBE_PROFILER_MESSAGE(Text)
#define CBE_PROFILER_MESSAGE_L(Text)
#define CBE_PROFILER_MESSAGE_C(Text, Colour)
#define CBE_PROFILER_MESSAGE_LC(Text, Colour)

#define CBE_PROFILER_ALLOC(Ptr, Size)
#define CBE_PROFILER_ALLOC_N(Ptr, Size, Name)
#define CBE_PROFILER_FREE(Ptr, Size)
#define CBE_PROFILER_FREE_N(Ptr, Size, Name)

#define CBE_PROFILER_ENTERFIBER(Text)
#define CBE_PROFILER_LEAVEFIBER()

// Below are for Persistent scopes
#define CBE_PROFILER_SCOPE_VAR_FULL(VarName, Name, ControlVar, Text, Colour, Value)
#define CBE_PROFILER_SCOPE_VAR_TC(VarName, Name, ControlVar, Text, Colour)
#define CBE_PROFILER_SCOPE_VAR_C(VarName, Name, ControlVar, Colour)
#define CBE_PROFILER_SCOPE_VAR(VarName, Name, ControlVar)

// Below are Transient variant of scopes
#define CBE_PROFILER_TSCOPE_VAR_FULL(VarName, Name, ControlVar, Text, Colour, Value)
#define CBE_PROFILER_TSCOPE_VAR_TC(VarName, Name, ControlVar, Text, Colour)
#define CBE_PROFILER_TSCOPE_VAR_C(VarName, Name, ControlVar, Colour)
#define CBE_PROFILER_TSCOPE_VAR(VarName, Name, ControlVar)

#endif // ENABLE_PROFILING

// For Persistent scope with Persistent lifetime for Names
#define CBE_PROFILER_SCOPE_DYN(Name, ControlVar) CBE_PROFILER_SCOPE_VAR(UNIQ_VAR_NAME(cbeProfilerScope_), Name, ControlVar)
#define CBE_PROFILER_SCOPE(Name) CBE_PROFILER_SCOPE_DYN(Name, true)
#define CBE_PROFILER_SCOPE_DYN_C(Name, ControlVar, Colour) CBE_PROFILER_SCOPE_VAR_C(UNIQ_VAR_NAME(cbeProfilerScope_), Name, ControlVar, Colour)
#define CBE_PROFILER_SCOPE_C(Name, Colour) CBE_PROFILER_SCOPE_DYN_C(Name, true, Colour)
#define CBE_PROFILER_SCOPE_DYN_TC(Name, ControlVar, Text, Colour)                                                                              \
    CBE_PROFILER_SCOPE_VAR_TC(UNIQ_VAR_NAME(cbeProfilerScope_), Name, ControlVar, Text, Colour)
#define CBE_PROFILER_SCOPE_TC(Name, Text, Colour) CBE_PROFILER_SCOPE_DYN_TC(Name, true, Text, Colour)

// For Transient scope
#define CBE_PROFILER_TSCOPE_DYN(Name, ControlVar) CBE_PROFILER_TSCOPE_VAR(UNIQ_VAR_NAME(cbeProfilerScope_), Name, ControlVar)
#define CBE_PROFILER_TSCOPE(Name) CBE_PROFILER_TSCOPE_DYN(Name, true)
#define CBE_PROFILER_TSCOPE_DYN_C(Name, ControlVar, Colour)                                                                                    \
    CBE_PROFILER_TSCOPE_VAR_C(UNIQ_VAR_NAME(cbeProfilerScope_), Name, ControlVar, Colour)
#define CBE_PROFILER_TSCOPE_C(Name, Colour) CBE_PROFILER_TSCOPE_DYN_C(Name, true, Colour)
#define CBE_PROFILER_TSCOPE_DYN_TC(Name, ControlVar, Text, Colour)                                                                             \
    CBE_PROFILER_TSCOPE_VAR_TC(UNIQ_VAR_NAME(cbeProfilerScope_), Name, ControlVar, Text, Colour)
#define CBE_PROFILER_TSCOPE_TC(Name, Text, Colour) CBE_PROFILER_TSCOPE_DYN_TC(Name, true, Text, Colour)