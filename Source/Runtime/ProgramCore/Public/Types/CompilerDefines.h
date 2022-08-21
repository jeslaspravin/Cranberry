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

// https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
#define WARN_UNUSED_FUNC_PARAM -Wunused-parameter
#define WARN_UNUSED_LOCAL_VARIABLE -Wunused-variable
#define WARN_UNUSED_LABEL -Wunused-label
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

// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/
#define WARN_UNUSED_FUNC_PARAM 4100
#define WARN_UNUSED_LOCAL_VARIABLE 4101
#define WARN_UNUSED_LABEL 4102
#define WARN_UNKNOWN_PRAGMAS
#define WARN_UNINITIALIZED
#define WARN_MISMATCHED_NEW_DELETE 4291
#define WARN_IMPLICIT_DESTRUCTOR_DELETE 4624
#define WARN_MISSING_OVERRIDE

#else
static_assert(false, "Unsupported compiler");
#endif

// clang-format on

#ifdef __cpp_lib_source_location
#define HAS_SOURCE_LOCATION_FEATURE __cpp_lib_source_location >= 201907L
#else
#define HAS_SOURCE_LOCATION_FEATURE 0
#endif

namespace CompilerHacks
{

template <typename T>
void ignoreUnused(T &&)
{}

} // namespace CompilerHacks
#define CALL_ONCE_internal(Func, VarQualifier)                                                                                                 \
    do                                                                                                                                         \
    {                                                                                                                                          \
        VarQualifier static int callOnce = (Func(), 1);                                                                                        \
        CompilerHacks::ignoreUnused(callOnce);                                                                                                 \
    }                                                                                                                                          \
    while (0)

#define CALL_ONCE(Func) CALL_ONCE_internal(Func, )
#define CALL_ONCE_PER_THREAD(Func) CALL_ONCE_internal(Func, thread_local)

#define DO_ONCE_internal(VarQualifier, ...)                                                                                                    \
    do                                                                                                                                         \
    {                                                                                                                                          \
        VarQualifier static int callOnce = (__VA_ARGS__, 1);                                                                                   \
        CompilerHacks::ignoreUnused(callOnce);                                                                                                 \
    }                                                                                                                                          \
    while (0)

#define DO_ONCE(...) DO_ONCE_internal(, __VA_ARGS__)
#define DO_ONCE_PER_THREAD(...) DO_ONCE_internal(thread_local, __VA_ARGS__)