/*!
 * \file StringFormat.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "String/String.h"
#include "Types/Delegates/Delegate.h"
#include "Types/Templates/TypeTraits.h"

#include <cstdio>
#include <sstream>
#include <unordered_map>

class StringFormat;

template <typename T>
concept HasValidCStrMethod = requires(T &&val)
{
    {
        val.c_str()
        } -> std::convertible_to<BaseString::const_pointer>;
};

template <typename T>
concept HasValidGetCharMethod = requires(T &&val)
{
    {
        val.getChar()
        } -> std::convertible_to<BaseString::const_pointer>;
};

// If not a string type and toString method exists
template <typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept HasToStringMethod = NonStringType<Type> && std::is_compound_v<CleanType> && requires(Type &&val)
{
    {
        val.toString()
        } -> std::convertible_to<String>;
};

template <typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept HasOStreamInsertOverrideMethod = NonStringType<Type> && std::is_compound_v<CleanType> && requires(Type &&val)
{
    {
        std::declval<OutputStream &>() << std::declval<Type>()
        } -> std::same_as<OutputStream &>;
};

template <typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept StringOrFundamentalTypes = std::disjunction_v<IsString<Type>, std::is_fundamental<CleanType>>;

template <typename Type, typename StringFormatType>
concept StringConvertibleIteratorType = NonStringType<Type> && requires(Type &&val)
{
    val.cbegin();
    val.cend();
    {
        StringFormatType::toString(*val.cbegin())
        } -> StringOrFundamentalTypes;
};

template <typename Type, typename StringFormatType>
concept HasStringFormatToStringImpl = requires(Type &&val)
{
    {
        StringFormatType::toString(val)
        } -> StringOrFundamentalTypes;
};

struct PROGRAMCORE_EXPORT FormatArg
{
    using ArgGetter = SingleCastDelegate<String>;
    union ArgValue
    {
        String strVal;
        ArgGetter argGetter;
        CoreTypesUnion fundamentalVals;

        ArgValue(CoreTypesUnion inFundamentalVals)
            : fundamentalVals(inFundamentalVals)
        {}
        template <typename StrType>
        requires StringType<StrType> ArgValue(StrType &&inStrVal)
            : strVal(std::forward<StrType>(inStrVal))
        {}
        ArgValue(const ArgGetter &getter)
            : argGetter(getter)
        {}
        ArgValue()
            : strVal()
        {}
        ~ArgValue() {}
    };
    ArgValue value;
    enum
    {
        NoType,
        Bool,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Int8,
        Int16,
        Int32,
        Int64,
        Float,
        Double,
        Getter,
        AsString // Whatever other type that will be stored as string after toString conversion
    } type;

    FormatArg()
        : value()
        , type(NoType)
    {}
    FormatArg(bool argValue)
        : type(Bool)
    {
        value.fundamentalVals.boolVal = argValue;
    }
    FormatArg(uint8 argValue)
        : type(UInt8)
    {
        value.fundamentalVals.uint8Val = argValue;
    }
    FormatArg(uint16 argValue)
        : type(UInt16)
    {
        value.fundamentalVals.uint16Val = argValue;
    }
    FormatArg(uint32 argValue)
        : type(UInt32)
    {
        value.fundamentalVals.uint32Val = argValue;
    }
    FormatArg(uint64 argValue)
        : type(UInt64)
    {
        value.fundamentalVals.uint64Val = argValue;
    }
    FormatArg(int8 argValue)
        : type(Int8)
    {
        value.fundamentalVals.int8Val = argValue;
    }
    FormatArg(int16 argValue)
        : type(Int16)
    {
        value.fundamentalVals.int16Val = argValue;
    }
    FormatArg(int32 argValue)
        : type(Int32)
    {
        value.fundamentalVals.int32Val = argValue;
    }
    FormatArg(int64 argValue)
        : type(Int64)
    {
        value.fundamentalVals.int64Val = argValue;
    }
    FormatArg(float argValue)
        : type(Float)
    {
        value.fundamentalVals.floatVal = argValue;
    }
    FormatArg(double argValue)
        : type(Double)
    {
        value.fundamentalVals.doubleVal = argValue;
    }
    FormatArg(ArgGetter argValue)
        : value(argValue)
        , type(Getter)
    {}
    template <typename InType>
    FormatArg(InType &&argValue);

    FormatArg(const FormatArg &arg);
    FormatArg(FormatArg &&arg);
    FormatArg &operator=(const FormatArg &arg);
    FormatArg &operator=(FormatArg &&arg);

    String toString() const;
    operator bool() const;
};
using FormatArgsMap = std::unordered_map<String, FormatArg>;

class StringFormat
{
public:
    // getChar - String to TChar* and primitive types pass templates
    template <typename StrType>
    requires HasValidCStrMethod<StrType> FORCE_INLINE CONST_EXPR static const TChar *getChar(const StrType &value)
    {
        return static_cast<const TChar *>(value.c_str());
    }

    template <typename StrType>
    requires HasValidGetCharMethod<StrType> &&(!HasValidCStrMethod<StrType>)FORCE_INLINE CONST_EXPR
        static const TChar *getChar(const StrType &value)
    {
        return static_cast<const TChar *>(value.getChar());
    }

    template <typename StrType>
    requires std::convertible_to<StrType, const TChar *> FORCE_INLINE CONST_EXPR static const TChar *getChar(StrType &&value)
    {
        return static_cast<const TChar *>(value);
    }

    template <typename StrType>
    requires NonStringType<StrType> FORCE_INLINE CONST_EXPR static StrType getChar(StrType &&value) { return std::forward<StrType>(value); }

    // toString - To String templates
    template <typename Type>
    requires HasToStringMethod<Type> FORCE_INLINE static String toString(const Type &value) { return value.toString(); }

    // Only std::ostream << type exists
    template <typename Type>
    requires HasOStreamInsertOverrideMethod<Type> FORCE_INLINE static BaseString toString(const Type &value)
    {
        OStringStream stream;
        stream << value;
        return stream.str();
    }

    // All string and primitive(fundamental) type
    template <typename Type>
    requires StringOrFundamentalTypes<Type> FORCE_INLINE CONST_EXPR static Type toString(Type &&value) { return std::forward<Type>(value); }

    template <typename KeyType, typename ValueType>
    requires HasStringFormatToStringImpl<KeyType, StringFormat> && HasStringFormatToStringImpl<ValueType, StringFormat>
        FORCE_INLINE static String toString(const std::pair<KeyType, ValueType> &pair)
    {
        OStringStream stream;
        stream << TCHAR("{ ") << toString(pair.first) << TCHAR(", ") << toString(pair.second) << TCHAR(" }");
        return stream.str();
    }

    template <typename IterableType>
    requires StringConvertibleIteratorType<IterableType, StringFormat> FORCE_INLINE static BaseString toString(IterableType &&iterable)
    {
        using Type = UnderlyingType<IterableType>;

        OStringStream stream;
        stream << TCHAR("[ ");
        typename Type::const_iterator itr = iterable.cbegin();
        if (itr != iterable.cend())
        {
            stream << toString(*itr);
            while ((++itr) != iterable.cend())
            {
                stream << TCHAR(", ") << toString(*itr);
            }
        }
        stream << TCHAR(" ]");
        return stream.str();
    }

private:
    StringFormat() = default;

#if USING_WIDE_UNICODE
#define STRING_PRINTF std::swprintf
#else // USING_WIDE_UNICODE
#define STRING_PRINTF std::snprintf
#endif
    // && (Not necessary but nice to have this)passes the type as it is from the caller like r-values as
    // well, else r-values gets converted to l-values on this call
    template <typename... Args>
    DEBUG_INLINE static String fmtString(const TChar *fmt, Args &&...args)
    {
        int32 size = STRING_PRINTF(nullptr, 0, fmt, getChar<Args>(std::forward<Args>(args))...);
        String fmted;
        // +1 size printf needs one more in buffer for null char
        fmted.resize(size + 1);
        STRING_PRINTF(fmted.data(), size + 1, fmt, getChar<Args>(std::forward<Args>(args))...);
        // Resizing it back to original length
        fmted.resize(size);
        return fmted;
    }
#undef STRING_PRINTF

public:
    template <typename FmtType, typename... Args>
    DEBUG_INLINE static String format(FmtType &&fmt, Args &&...args)
    {
        return fmtString(getChar<FmtType>(std::forward<FmtType>(fmt)), toString<Args>(std::forward<Args>(args))...);
    }

    PROGRAMCORE_EXPORT static String formatMustache(const String &fmt, const FormatArgsMap &formatArgs);
};

template <typename InType>
FormatArg::FormatArg(InType &&argValue)
    : value(StringFormat::toString<InType>(std::forward<InType>(argValue)))
    , type(FormatArg::AsString)
{}
