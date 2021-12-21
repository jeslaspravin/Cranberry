#pragma once

#define ARRAY_LENGTH(ArrayVar) sizeof(ArrayVar)/sizeof(ArrayVar[0])
#define MACRO_TO_STRING_internal(DefExpanded) #DefExpanded
#define MACRO_TO_STRING(VarName) MACRO_TO_STRING_internal(VarName)

// If only one bit set in this unsigned integer
#define ONE_BIT_SET(FlagStatement) ((FlagStatement) && !((FlagStatement) & ((FlagStatement) - 1)))
// If all check bits is set in this unsigned integer
#define BIT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) == CheckFlags)
// If not all check bits is set in this unsigned integer
#define BIT_NOT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) != CheckFlags)
// If any bit of check bits is set in this unsigned integer
#define ANY_BIT_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) > 0)
// If no bits of check bits is set in this unsigned integer
#define NO_BITS_SET(FlagStatement, CheckFlags) (((FlagStatement) & (CheckFlags)) > 0)
#define INDEX_TO_FLAG_MASK(Idx) (1 << (Idx))

#define MAKE_INITIALIZER(...) { __VA_ARGS__ }

#define FIRST_internal(X,...) X
#define TUPLE_TAIL_internal(X,...) __VA_ARGS__
#define VAR_COUNT_internal(Var1,Var2,Var3,Var4,Var5,Var6,Var7,Var8,Var9,VarCount, ...) VarCount
#define COMBINE_internal(X,Y) X##Y

#define EXPAND_ARGS(...) __VA_ARGS__
#define FIRST(...) EXPAND_ARGS(FIRST_internal(__VA_ARGS__))
#define TUPLE_TAIL(...) EXPAND_ARGS(TUPLE_TAIL_internal(__VA_ARGS__))
#define VAR_COUNT(...) EXPAND_ARGS(VAR_COUNT_internal(__VA_ARGS__, 9,8,7,6,5,4,3,2,1))
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