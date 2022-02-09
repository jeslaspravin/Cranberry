/*!
 * \file WindowsPlatformDefines.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

// String defines
#ifndef USING_UNICODE
#define USING_UNICODE 1
#endif
#ifndef TCHAR_inner
#define TCHAR_inner(x) L##x
#endif

// Other platform specific

#ifndef FORCE_INLINE
#if (_MSC_VER >= 1200)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE __inline
#endif
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 0
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

#ifndef LINE_FEED_CHAR
#define LINE_FEED_CHAR TCHAR("\r\n")
#endif

// File System path separator
#ifndef FS_PATH_SEPARATOR
#define FS_PATH_SEPARATOR TCHAR("\\")
#endif