#pragma once

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


#ifndef DLL_EXPORT
// DLL export and import
#define DLL_EXPORT __declspec(dllexport)
#endif
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif