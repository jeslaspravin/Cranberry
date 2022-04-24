/*!
 * \file CompilerDefines.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreMiscDefines.h"

// Aggregated blog -
// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html

// Clang defines has to be first before anything else as reflection parsing(uses clang) needs it to
// expand valid macros

// clang-format off
#if defined __clang__ | defined __GNUC__ | defined __REF_PARSE__
#define COMPILER_MAJOR_VER __clang_major__
#define COMPILER_MINOR_VER __clang_minor__
#define COMPILER_PATCH_VER __clang_patchlevel__

#define COMPILER_PRAGMA(MacroText) _Pragma(MACRO_TO_STRING(MacroText))

#define COMPILER_MESSAGE(MsgStr) message MsgStr

#define COMPILER_PUSH_WARNING clang diagnostic push
#define COMPILER_DISABLE_WARNING(WarningMacro) clang diagnostic ignored MACRO_TO_STRING(WarningMacro)
#define COMPILER_POP_WARNING clang diagnostic pop

#define WARN_UNKNOWN_PRAGMAS -Wunknown-pragmas
#define WARN_UNINITIALIZED -Wuninitialized
#define WARN_MISMATCHED_NEW_DELETE -Wmismatched-new-delete
#define WARN_IMPLICIT_DESTRUCTOR_DELETE
#define WARN_MISSING_OVERRIDE -Winconsistent-missing-override

#elif defined _MSC_VER
#define COMPILER_MAJOR_VER _MSC_VER
#define COMPILER_MINOR_VER 0
#define COMPILER_PATCH_VER 0

#define COMPILER_PRAGMA(MacroText) __pragma(MacroText)

#define COMPILER_MESSAGE(MsgStr) message(MsgStr)

#define COMPILER_PUSH_WARNING warning(push)
#define COMPILER_DISABLE_WARNING(WarningMacro) warning(disable : WarningMacro)
#define COMPILER_POP_WARNING warning(pop)

#define WARN_UNKNOWN_PRAGMAS
#define WARN_UNINITIALIZED
#define WARN_MISMATCHED_NEW_DELETE 4291
#define WARN_IMPLICIT_DESTRUCTOR_DELETE 4624
#define WARN_MISSING_OVERRIDE

#else
static_assert(false, "Unsupported compiler");
#endif

// clang-format on