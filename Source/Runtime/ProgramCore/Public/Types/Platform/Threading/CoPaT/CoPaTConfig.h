#pragma once

/*!
 * \file CoPaTConfig.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"
#include "Profiler/ProgramProfiler.hpp"
#include "Memory/Memory.h"
#include "Types/Delegates/Delegate.h"
#include "Types/Platform/Threading/PlatformThreading.h"
#include "Types/Platform/Threading/SyncPrimitives.h"
#include "Types/Platform/PlatformAssertionErrors.h"

/**
 * Thread types that are added by user
 */
// #define FOR_EACH_THREAD_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName)    \
//      FirstMacroName(Thread1)                                                                 \
//      MacroName(Thread2)                                                                      \
//      ...                                                                                     \
//      LastMacroName(ThreadN)
//
// #define FOR_EACH_THREAD_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) FirstMacroName(RenderThread)
#define FOR_EACH_UDTHREAD_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) FirstMacroName(RenderThread)
#define FOR_EACH_UDTHREAD_TYPES(MacroName) FOR_EACH_UDTHREAD_TYPES_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

#define USER_DEFINED_THREAD(ThreadType) ThreadType,
#define USER_DEFINED_THREADS() FOR_EACH_UDTHREAD_TYPES(USER_DEFINED_THREAD)

/**
 * Define Cache line size override here
 */
#define OVERRIDE_CACHE_LINE_SIZE CACHELINE_SIZE

/**
 * Override memory allocator type
 */
// #define OVERRIDE_MEMORY_ALLOCATOR AllocatorType
#define OVERRIDE_MEMORY_ALLOCATOR CBEMemory

/**
 * Defines Export symbols macro in case using this inside shared library
 */
#define OVERRIDE_EXPORT_SYM PROGRAMCORE_EXPORT

/**
 * If you want to use your custom spin lock(Note that it must have proper class and function signature like in SpinLock in SyncPrimitives.h
 */
// #define OVERRIDE_SPINLOCK CustomSpinLock
#define OVERRIDE_SPINLOCK CBESpinLock

/**
 * Define if we do not want to have everything inside namespace
 */
// #define WRAP_INSIDE_NS 0

/**
 * Override for assert macro.
 */
// #define OVERRIDE_ASSERT(expr) assert((expr))
#define OVERRIDE_ASSERT(expr) debugAssert((expr))

/**
 * Override promise::unhandled_exception() handler
 */
// #define OVERRIDE_UNHANDLED_EXCEPT_HANDLER() unhandledexception_handler::crash()
#define OVERRIDE_UNHANDLED_EXCEPT_HANDLER() fatalAssert(!"Coroutine unhandled exception")

/**
 * Override Function store type, expects signature FunctionType<ReturnType, Args...>
 */
// #define OVERRIDE_FUNCTION_TYPE std::function
#define OVERRIDE_FUNCTION_TYPE SingleCastDelegate

/**
 * Override char type?
 * Override char specifier macro and to string macro.
 */
// #define OVERRIDE_TCHAR_TYPE wchar_t
// #define OVERRIDE_TCHAR(expr) L##expr
// #define OVERRIDE_TOSTRING(expr) std::to_wstring(expr)
#define OVERRIDE_TCHAR(expr) TCHAR(expr)
#define OVERRIDE_TOSTRING(expr) String::toString(expr)

/**
 * Override for profiler char
 * Override for profiler scope macro. Name will be static char
 */
// #define OVERRIDE_PROFILER_CHAR(expr)
// #define OVERRIDE_PROFILER_SCOPE(Name)
// #define OVERRIDE_PROFILER_SCOPE_VALUE(Name, Value)
#define OVERRIDE_PROFILER_CHAR(expr) CBE_PROFILER_CHAR(expr)
#define OVERRIDE_PROFILER_SCOPE(Name) CBE_PROFILER_SCOPE(Name)
#define OVERRIDE_PROFILER_SCOPE_VALUE(Name, Value) CBE_PROFILER_SCOPE_VC(Name, CBE_PROFILER_DEFAULT_COLOR, Value)

/**
 * Override PlatformThreadingFunctions.
 */
// #define OVERRIDE_PLATFORMTHREADINGFUNCTIONS YourPlatformFunctions
#define OVERRIDE_PLATFORMTHREADINGFUNCTIONS PlatformThreadingFunctions

// If enable FAAArrayQueue node allocations tracking
// #define COPAT_ENABLE_QUEUE_ALLOC_TRACKING 1
#define COPAT_ENABLE_QUEUE_ALLOC_TRACKING 1

/**
 * Override uint32_t and uint64_t?
 */
// #define OVERRIDE_UINT32 uint32
// #define OVERRIDE_UINT64 uint64
#define OVERRIDE_UINT32 uint32
#define OVERRIDE_UINT64 uint64

//////////////////////////////////////////////////////////////////////////
/// Actual config
//////////////////////////////////////////////////////////////////////////

#ifndef USER_DEFINED_THREADS
#define USER_DEFINED_THREADS()
#endif

#if (defined OVERRIDE_CACHE_LINE_SIZE & OVERRIDE_CACHE_LINE_SIZE != 0)
#define CACHE_LINE_SIZE OVERRIDE_CACHE_LINE_SIZE
#else
#define CACHE_LINE_SIZE 64
#endif

#ifdef OVERRIDE_EXPORT_SYM
#define COPAT_EXPORT_SYM OVERRIDE_EXPORT_SYM
#else
// By default we use this library as static lib or embedded code
#define COPAT_EXPORT_SYM
#endif

#ifndef WRAP_INSIDE_NS
#define WRAP_INSIDE_NS 1
#endif

#if WRAP_INSIDE_NS
#define COPAT_NS_INLINED
#else
#define COPAT_NS_INLINED inline
#endif

#ifdef OVERRIDE_ASSERT
#define COPAT_ASSERT(expr) OVERRIDE_ASSERT(expr)
#else
#include <assert.h>
#define COPAT_ASSERT(expr) assert((expr))
#endif

#ifdef OVERRIDE_UNHANDLED_EXCEPT_HANDLER
#define COPAT_UNHANDLED_EXCEPT() OVERRIDE_UNHANDLED_EXCEPT_HANDLER()
#else
#define COPAT_UNHANDLED_EXCEPT() COPAT_ASSERT(!COPAT_TCHAR("CoPaT unhandled exception"))
#endif

#ifdef OVERRIDE_TCHAR
#define COPAT_TCHAR(x) OVERRIDE_TCHAR(x)
#else
#define COPAT_TCHAR(x) x
#endif

#ifdef OVERRIDE_TOSTRING
#define COPAT_TOSTRING(x) OVERRIDE_TOSTRING(x)
#else
#define COPAT_TOSTRING(x) std::to_string(x)
#endif

#ifdef OVERRIDE_PROFILER_SCOPE
#define COPAT_PROFILER_SCOPE(Name) OVERRIDE_PROFILER_SCOPE(Name)
#else
#define COPAT_PROFILER_SCOPE(Name)
#endif

#ifdef OVERRIDE_PROFILER_SCOPE_VALUE
#define COPAT_PROFILER_SCOPE_VALUE(Name, Value) OVERRIDE_PROFILER_SCOPE_VALUE(Name, Value)
#else
#define COPAT_PROFILER_SCOPE_VALUE(Name, Value)
#endif

#ifdef OVERRIDE_PROFILER_CHAR
#define COPAT_PROFILER_CHAR(x) OVERRIDE_PROFILER_CHAR(x)
#else
#define COPAT_PROFILER_CHAR(x) COPAT_TCHAR(x)
#endif

#ifndef COPAT_ENABLE_QUEUE_ALLOC_TRACKING

#ifdef _DEBUG
#define COPAT_ENABLE_QUEUE_ALLOC_TRACKING 1
#else
#define COPAT_ENABLE_QUEUE_ALLOC_TRACKING 0
#endif

#endif // #ifndef COPAT_ENABLE_QUEUE_ALLOC_TRACKING

COPAT_NS_INLINED
namespace copat
{

#ifdef OVERRIDE_UINT32
using u32 = OVERRIDE_UINT32;
#else
using u32 = uint32_t;
#endif

#ifdef OVERRIDE_UINT64
using u64 = OVERRIDE_UINT64;
#else
using u64 = uint64_t;
#endif

#ifdef OVERRIDE_TCHAR_TYPE
using TChar = OVERRIDE_TCHAR_TYPE;
#else
using TChar = std::remove_cv_t<std::remove_pointer_t<decltype(COPAT_TOSTRING(0).c_str())>>;
#endif

#ifdef OVERRIDE_FUNCTION_TYPE
template <typename RetType, typename... Args>
using FunctionType = OVERRIDE_FUNCTION_TYPE<RetType, Args...>;
#else
template <typename RetType, typename... Args>
using FunctionType = std::function<RetType(Args...)>;
#endif

} // namespace copat