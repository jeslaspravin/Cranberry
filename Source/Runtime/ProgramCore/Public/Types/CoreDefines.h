#pragma once

#include "Types/Platform/PlatformDefines.h"

#ifndef FORCE_INLINE
#define FORCE_INLINE inline
#endif

// Debug-able inline
#define DEBUG_INLINE inline

#define ARRAY_LENGTH(ArrayVar) sizeof(ArrayVar)/sizeof(ArrayVar[0])

#define ONE_BIT_SET(FlagStatement) ((FlagStatement) && !((FlagStatement) & ((FlagStatement) - 1)))
#define BIT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) == CheckFlags)
#define BIT_NOT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) != CheckFlags)
#define ANY_BIT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) > 0)

#define MAKE_INITIALIZER(...) { __VA_ARGS__ }

#define FIRST_internal(X,...) X
#define TUPLE_TAIL_internal(X,...) __VA_ARGS__
#define VAR_COUNT_internal(Var1,Var2,Var3,Var4,Var5,Var6,Var7,Var8,Var9,VarCount, ...) VarCount
#define COMBINE_internal(X,Y) X##Y

#define ExpandArgs(...) __VA_ARGS__
#define FIRST(...) ExpandArgs(FIRST_internal(__VA_ARGS__))
#define TUPLE_TAIL(...) ExpandArgs(TUPLE_TAIL_internal(__VA_ARGS__))
#define VAR_COUNT(...) ExpandArgs(VAR_COUNT_internal(__VA_ARGS__, 9,8,7,6,5,4,3,2,1))
#define COMBINE(X,Y) COMBINE_internal(X,Y)


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