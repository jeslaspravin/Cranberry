/*!
 * \file CoreMiscDefines.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#ifndef DEVELOPMENT
#define DEVELOPMENT 0
#endif

#if _DEBUG
#define DEBUG_BUILD 1
#define DEV_BUILD 1
#define DEBUG_VALIDATIONS 1
#elif DEVELOPMENT
#define DEV_BUILD 1
#define DEBUG_VALIDATIONS 1
#elif NDEBUG
#define RELEASE_BUILD 1
#endif

#ifndef DEV_BUILD
#define DEV_BUILD 0
#endif

#ifndef DEBUG_BUILD
#define DEBUG_BUILD 0
#endif

#ifndef RELEASE_BUILD
#define RELEASE_BUILD 0
#endif

/**
 * If all libraries are built for editor
 */
#ifndef EDITOR_BUILD
#define EDITOR_BUILD 0
#endif

#ifndef DEBUG_VALIDATIONS
#define DEBUG_VALIDATIONS 0
#endif

#ifndef LOG_TO_CONSOLE
#define LOG_TO_CONSOLE 0
#endif

#ifndef ENABLE_VERBOSE_LOG
#define ENABLE_VERBOSE_LOG 0
#endif

#ifndef HAS_ANY_PROFILER
#define HAS_ANY_PROFILER 0
#endif

#ifndef ENABLE_PROFILING
#define ENABLE_PROFILING HAS_ANY_PROFILER
#endif // ENABLE_PROFILING

#define ARRAY_LENGTH(ArrayVar) (sizeof(ArrayVar) / sizeof(ArrayVar[0]))
#define MACRO_TO_STRING_internal(DefExpanded) #DefExpanded
#define MACRO_TO_STRING(VarName) MACRO_TO_STRING_internal(VarName)

#define MAKE_UNIQUE_VARNAME(BaseName) COMBINE(BaseName, COMBINE(_, __COUNTER__))

#define CODE_BLOCK_BODY(...)                                                                                                                   \
    {                                                                                                                                          \
        __VA_ARGS__                                                                                                                            \
    }

#define MAKE_TYPE_NONCOPY_NONMOVE(TypeName)                                                                                                    \
    TypeName(TypeName &&) = delete;                                                                                                            \
    TypeName(const TypeName &) = delete;                                                                                                       \
    TypeName &operator= (TypeName &&) = delete;                                                                                                \
    TypeName &operator= (const TypeName &) = delete;

#define MAKE_TYPE_DEFAULT_COPY_MOVE(TypeName)                                                                                                  \
    TypeName(TypeName &&) = default;                                                                                                           \
    TypeName(const TypeName &) = default;                                                                                                      \
    TypeName &operator= (TypeName &&) = default;                                                                                               \
    TypeName &operator= (const TypeName &) = default;

// If only one bit set in this unsigned integer
#define ONE_BIT_SET(FlagStatement) ((FlagStatement) && !((FlagStatement) & ((FlagStatement)-1)))
// If all check bits is set in this unsigned integer
#define BIT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) == (CheckFlags))
// If not all check bits is set in this unsigned integer
#define BIT_NOT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) != (CheckFlags))
// If any bit of check bits is set in this unsigned integer
#define ANY_BIT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) > 0)
// If no bits of check bits is set in this unsigned integer
#define NO_BITS_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) == 0)
#define INDEX_TO_FLAG_MASK(Idx) (decltype(Idx)(1) << (Idx))
// Sets all bits that are set in value and mask or already set
#define SET_BITS_MASKED(SetTo, ValueFlags, FlagsMask) (SetTo) |= ((ValueFlags) & (FlagsMask))
#define SET_BITS(SetTo, FlagsMask) (SetTo) |= (FlagsMask)
#define CLEAR_BITS(SetTo, FlagsMask) (SetTo) &= ~(FlagsMask)
#define SET_BIT_AT(SetTo, AtIdx) (SetTo) |= INDEX_TO_FLAG_MASK(AtIdx)
#define CLEAR_BIT_AT(SetTo, AtIdx) (SetTo) &= ~INDEX_TO_FLAG_MASK(AtIdx)
// Replaces all masked region with provided value, Unmasked bits are not touched
#define REPLACE_BITS_MASKED(SetTo, ValueFlags, FlagsMask) (SetTo) = ((SetTo) & ~(FlagsMask)) | ((ValueFlags) & (FlagsMask))

#ifdef __COUNTER__
#define UNIQ_VAR_NAME(NamePrefix) COMBINE(NamePrefix, __COUNTER__)
#else
#define UNIQ_VAR_NAME(NamePrefix) COMBINE(NamePrefix, __LINE__)
#endif

#define MAKE_INITIALIZER(...) MAKE_INITIALIZER_internal(__VA_ARGS__)

#define EXPAND_ARGS(...) __VA_ARGS__
#define IGNORE_ARGS(...)

#define FIRST(...) EXPAND_ARGS(FIRST_internal(__VA_ARGS__))
#define TUPLE_TAIL(...) EXPAND_ARGS(TUPLE_TAIL_internal(__VA_ARGS__))
#define VAR_COUNT(...) EXPAND_ARGS(VAR_COUNT_internal(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1))
#define COMBINE(X, Y) COMBINE_internal(X, Y)

#define TRANSFORM_1(Callable, ...) Callable(FIRST(__VA_ARGS__))
#define TRANSFORM_2(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_1(Callable, TUPLE_TAIL(__VA_ARGS__))
#define TRANSFORM_3(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_2(Callable, TUPLE_TAIL(__VA_ARGS__))
#define TRANSFORM_4(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_3(Callable, TUPLE_TAIL(__VA_ARGS__))
#define TRANSFORM_5(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_4(Callable, TUPLE_TAIL(__VA_ARGS__))
#define TRANSFORM_6(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_5(Callable, TUPLE_TAIL(__VA_ARGS__))
#define TRANSFORM_7(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_6(Callable, TUPLE_TAIL(__VA_ARGS__))
#define TRANSFORM_8(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_7(Callable, TUPLE_TAIL(__VA_ARGS__))
#define TRANSFORM_9(Callable, ...) Callable(FIRST(__VA_ARGS__)), TRANSFORM_8(Callable, TUPLE_TAIL(__VA_ARGS__))
// Can support up to 9 Items only
#define TRANSFORM_ALL(Callable, ...) COMBINE(TRANSFORM_, VAR_COUNT(__VA_ARGS__))(Callable, __VA_ARGS__)

#define MAKE_INITIALIZER_internal(...)                                                                                                         \
    {                                                                                                                                          \
        __VA_ARGS__                                                                                                                            \
    }
#define FIRST_internal(X, ...) X
#define TUPLE_TAIL_internal(X, ...) __VA_ARGS__
#define VAR_COUNT_internal(Var1, Var2, Var3, Var4, Var5, Var6, Var7, Var8, Var9, VarCount, ...) VarCount
#define COMBINE_internal(X, Y) X##Y