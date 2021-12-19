#pragma once

#include "Types/Platform/PlatformDefines.h"
#include "Types/CoreMiscDefines.h"

// Platform safety defines

#ifndef FORCE_INLINE
#define FORCE_INLINE inline
#endif

// Debug-able inline
#if _DEBUG
#define DEBUG_INLINE
#elif NDEBUG
#define DEBUG_INLINE FORCE_INLINE
#else
#define DEBUG_INLINE inline
#endif

#ifndef LIB_PREFIX
#define LIB_PREFIX
#endif
#ifndef SHARED_LIB_EXTENSION
#define SHARED_LIB_EXTENSION
#endif
#ifndef STATIC_LIB_EXTENSION
#define STATIC_LIB_EXTENSION
#endif

#ifndef DLL_EXPORT
// DLL export and import
#define DLL_EXPORT
#endif
#ifndef DLL_IMPORT
#define DLL_IMPORT
#endif

#ifndef LINE_FEED_CHAR
#define LINE_FEED_CHAR "\n"
#endif

// File System path separator
#ifndef FS_PATH_SEPARATOR
#define FS_PATH_SEPARATOR "/"
#endif

// Other non platform defines

// CPP Language specific defines

#ifndef CONST_EXPR
#define CONST_EXPR constexpr
#endif

#ifndef CONST_EVAL
#define CONST_EVAL consteval
#endif

#ifndef CONST_INIT
#define CONST_INIT constinit
#endif

#ifndef NODISCARD
#define NODISCARD [[nodiscard]]
#endif

#ifndef DEPRECATED
#define DEPRECATED(Message) [[deprecated( #Message )]]
#endif

// End CPP language specific

#ifndef LOG_TO_CONSOLE
#define LOG_TO_CONSOLE 0
#endif