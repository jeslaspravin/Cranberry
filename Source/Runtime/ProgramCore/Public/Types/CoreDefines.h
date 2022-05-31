/*!
 * \file CoreDefines.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreMiscDefines.h"
#include "Types/Platform/PlatformDefines.h"

// Platform safety defines

#ifndef FORCE_INLINE
#define FORCE_INLINE inline
#endif

// String defines
// Always use TCHAR to initialize represent inline char arrays, always use like TCHAR("some chars" )
// Changing UNICODE must also be changed in TChar typedef of CoreTypes
// If using wide unicode it means we are using platform native Unicode in this platform as TChar
#ifndef USING_WIDE_UNICODE
#define USING_WIDE_UNICODE 0
#endif
#ifndef TCHAR_inner
#define TCHAR_inner(x) x
#endif
#define TCHAR(x) TCHAR_inner(x)

// Debug-able inline
#if DEBUG_BUILD
#define DEBUG_INLINE
#elif RELEASE_BUILD
#define DEBUG_INLINE FORCE_INLINE
#else
#define DEBUG_INLINE inline
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 0
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

#ifndef LINE_FEED_ACHAR
#define LINE_FEED_ACHAR "\n"
#endif
#ifndef LINE_FEED_TCHAR
#define LINE_FEED_TCHAR TCHAR(LINE_FEED_ACHAR)
#endif

// File System path separator
#ifndef FS_PATH_SEPARATOR
#define FS_PATH_SEPARATOR TCHAR("/")
#endif

// From https://stackoverflow.com/questions/35566315/offset-of-type-in-multiple-inheritance using 1 because 0 means nullptr and calculation will
// be invalid Offset for a class's VTable in case of multiple inheritance
#ifndef MI_VTABLE_OFFSET
#define MI_VTABLE_OFFSET(ClassTypeName, OffsetOfClassTypeName) (((PtrInt) static_cast<OffsetOfClassTypeName *>((ClassTypeName *)1)) - 1)
#endif

#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif

// Math defines
#ifndef IS_FINITE
#define IS_FINITE std::isfinite
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
#define DEPRECATED(Message) [[deprecated(#Message)]]
#endif

#ifndef STATIC_LINKED
#define STATIC_LINKED 0
#endif

// End CPP language specific