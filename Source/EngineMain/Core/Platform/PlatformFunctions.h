#pragma once

#include <type_traits>

#if _WIN32

#include "Windows/WindowsPlatformFunctions.h"

#elif __unix__

static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif

typedef GPlatformFunctions::PlatformFunctions PlatformFunctions;

#define ONE_BIT_SET(FlagStatement) (FlagStatement) && !((FlagStatement) & ((FlagStatement) - 1))


// Preserves the bits while conversion
template<typename SignedType, typename UnsignedType, bool IncludeSignBit>
struct TypeConversion
{

};

// Preserves the bits while conversion
template<typename SignedType, typename UnsignedType>
struct TypeConversion<SignedType, UnsignedType, true>
{
	constexpr static UnsignedType typeMaskSigned()
	{
		return UnsignedType(1) << ((sizeof(SignedType{}) * 8) - 1);
	}

	constexpr static UnsignedType typeMaskUnsigned()
	{
		return UnsignedType(1) << ((sizeof(UnsignedType{}) * 8) - 1);
	}

	static typename std::enable_if<std::is_signed<SignedType>::value, UnsignedType>::type toUnsigned(const SignedType& value)
	{
		return (value & typeMaskSigned()) > 0 ? ((UnsignedType)-value) | typeMaskSigned() : value;
	}

	static typename std::enable_if<std::is_unsigned<UnsignedType>::value, SignedType>::type toSigned(const UnsignedType& value)
	{
		return (value & typeMaskUnsigned()) > 0 ? getSignedValueInternal(value) : value;
	}

	inline static SignedType getSignedValueInternal(const UnsignedType& value)
	{
		if constexpr(sizeof(SignedType{}) == sizeof(UnsignedType{}))
		{
			return -((SignedType)(value & ~typeMaskUnsigned()));
		}
		else if constexpr(sizeof(SignedType{}) > sizeof(UnsignedType{}))
		{
			return ((SignedType)(value & ~typeMaskUnsigned())) | typeMaskUnsigned();
		}
		else
		{
			return (SignedType)(value & ~typeMaskUnsigned());// Data loss
		}
	}
};

// Preserves the bits while conversion
template<typename SignedType, typename UnsignedType>
struct TypeConversion<SignedType, UnsignedType, false>
{
	constexpr static UnsignedType typeMaskSigned()
	{
		return 1 << ((sizeof(UnsignedType{}) * 8) - 1);
	}

	constexpr static UnsignedType typeMaskUnsigned()
	{
		return 1 << ((sizeof(UnsignedType{}) * 8) - 1);
	}

	static typename std::enable_if<std::is_signed<SignedType>::value, UnsignedType>::type toUnsigned(const SignedType& value)
	{
		return (value & typeMaskSigned()) > 0 ? ((UnsignedType)-value) : value;
	}

	static typename std::enable_if<std::is_unsigned<UnsignedType>::value, SignedType>::type toSigned(const UnsignedType& value)
	{
		return (value & typeMaskUnsigned()) > 0 ? (SignedType)(value & ~typeMaskUnsigned()) : value;
	}
};
