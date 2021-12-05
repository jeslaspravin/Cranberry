#pragma once

#include "Config/EngineVariableTypes.h"
#include "GenericPlatformFunctions.h"

#if PLATFORM_WINDOWS

#include "WindowsPlatformFunctions.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif

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

enum class PROGRAMCORE_EXPORT EndianType
{
    Big,
    Little
};

class PROGRAMCORE_EXPORT PlatformEndian : public EngineVar<EndianType>
{
public:
    PlatformEndian();

    bool isBigEndian() const;
    bool isLittleEndian() const;
};

typedef GPlatformFunctions::PlatformFunctions PlatformFunctions;

namespace GPlatformConfigs
{
    PROGRAMCORE_EXPORT extern PlatformEndian PLATFORM_ENDIAN;
}