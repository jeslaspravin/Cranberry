/*!
 * \file WindowsPlatformDefines.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

// String defines
// Right now using UTF-8 for windows as windows do not recommend UTF-16 anymore, If changing here also change TChar in CoreTypes.h
#ifndef USING_WIDE_UNICODE
#define USING_WIDE_UNICODE 0
#endif
#ifndef TCHAR_inner
#if USING_WIDE_UNICODE
#define TCHAR_inner(x) L##x
#else // USING_WIDE_UNICODE
#define TCHAR_inner(x) x
#endif // USING_WIDE_UNICODE
#endif

// Other platform specific

#ifndef FORCE_INLINE
#if (_MSC_VER >= 1200)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE __inline
#endif
#endif

// Aligned platform specific memory allocators macro
// TODO(Jeslas) : Check if this is MSVC specific or specific to Windows(Found it in windows sdk so
// probably windows specific check however)
#ifndef PLATFORM_ALIGNED_MALLOC
#define PLATFORM_ALIGNED_MALLOC _aligned_malloc
#define PLATFORM_ALIGNED_REALLOC _aligned_realloc
#define PLATFORM_ALIGNED_FREE _aligned_free
#endif

// Windows has no library prefix
#ifndef LIB_PREFIX
#define LIB_PREFIX TCHAR("")
#endif
#ifndef SHARED_LIB_EXTENSION
#define SHARED_LIB_EXTENSION TCHAR("dll")
#endif
#ifndef STATIC_LIB_EXTENSION
#define STATIC_LIB_EXTENSION TCHAR("lib")
#endif

#ifndef DLL_EXPORT
// DLL export and import
#define DLL_EXPORT __declspec(dllexport)
#endif
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif

#ifndef LINE_FEED_ACHAR
#define LINE_FEED_ACHAR "\r\n"
#endif

// File System path separator
#ifndef FS_PATH_SEPARATOR
#define FS_PATH_SEPARATOR TCHAR("\\")
#endif