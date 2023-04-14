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
#include "String/StringLiteral.h"
#include "Types/Templates/TypeTraits.h"

#include <cstdio>
#include <sstream>
#include <format>

class StringFormat;

template <typename T>
concept HasValidCStrMethod = requires(T &&val) {
    {
        val.c_str()
    } -> std::convertible_to<BaseString::const_pointer>;
};

template <typename T>
concept HasValidGetCharMethod = requires(T &&val) {
    {
        val.getChar()
    } -> std::convertible_to<BaseString::const_pointer>;
};

// If not a string type and toString method exists
template <typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept HasToStringMethod = NonStringType<Type> && std::is_compound_v<CleanType> && requires(Type &&val) {
    {
        val.toString()
    } -> std::convertible_to<String>;
};

template <typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept HasOStreamInsertOverrideMethod = NonStringType<Type> && std::is_compound_v<CleanType> && requires(Type &&val, OutputStream &stream) {
    {
        stream << val
    } -> std::same_as<OutputStream &>;
};

template <typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept StringOrFundamentalTypes = std::disjunction_v<IsString<Type>, std::is_fundamental<CleanType>>;

template <typename Type, typename StringFormatType>
concept StringConvertibleIteratorType = NonStringType<Type> && requires(Type &&val) {
    val.cbegin();
    val.cend();
    {
        StringFormatType::toString(*val.cbegin())
    } -> StringOrFundamentalTypes;
};

template <typename Type, typename StringFormatType>
concept HasStringFormatToStringImpl = requires(Type &&val) {
    {
        StringFormatType::toString(val)
    } -> StringOrFundamentalTypes;
};

// std::format related types and concepts
using StdFormatContextType = std::conditional_t<IsTCharWide::value, std::wformat_context, std::format_context>;
template <class Type>
concept HasStdFormatter = requires(Type value, StdFormatContextType &context) {
    std::declval<typename StdFormatContextType::template formatter_type<std::remove_cvref_t<Type>>>().format(value, context);
};
// Specialization to support String in std::format
template <typename CharType>
struct std::formatter<String, CharType> : std::formatter<BaseString, CharType>
{
public:
    using BaseType = std::formatter<BaseString, CharType>;

    template <class FormatContext>
    typename FormatContext::iterator format(const String &value, FormatContext &formatCtx) const
    {
        return BaseType::format(value, formatCtx);
    }
};

class StringFormat
{
public:
    // getChar - String to TChar* and primitive types pass templates
    template <typename StrType>
    requires HasValidCStrMethod<StrType>
    constexpr static const TChar *getChar(const StrType &value)
    {
        return static_cast<const TChar *>(value.c_str());
    }

    template <typename StrType>
    requires HasValidGetCharMethod<StrType> && (!HasValidCStrMethod<StrType>)
    constexpr static const TChar *getChar(const StrType &value)
    {
        return static_cast<const TChar *>(value.getChar());
    }

    template <typename StrType>
    requires std::convertible_to<StrType, const TChar *>
    constexpr static const TChar *getChar(StrType value)
    {
        return static_cast<const TChar *>(value);
    }

    // Fall through getChar()
    template <typename StrType>
    constexpr static StrType getChar(StrType value)
    {
        return std::forward<StrType>(value);
    }

    // toString - To String templates
    template <typename Type>
    requires HasToStringMethod<Type>
    FORCE_INLINE static String toString(const Type &value)
    {
        return value.toString();
    }

    // Only std::ostream << type exists
    template <typename Type>
    requires HasOStreamInsertOverrideMethod<Type>
    FORCE_INLINE static BaseString toString(const Type &value)
    {
        OStringStream stream;
        stream << value;
        return stream.str();
    }

    // All string and primitive(fundamental) type
    template <typename Type>
    requires StringOrFundamentalTypes<Type>
    constexpr static Type toString(Type &&value)
    {
        return std::forward<Type>(value);
    }

    template <typename KeyType, typename ValueType>
    requires HasStringFormatToStringImpl<KeyType, StringFormat> && HasStringFormatToStringImpl<ValueType, StringFormat>
    static String toString(const std::pair<KeyType, ValueType> &pair)
    {
        OStringStream stream;
        stream << TCHAR("{ ") << toString(pair.first) << TCHAR(", ") << toString(pair.second) << TCHAR(" }");
        return stream.str();
    }

    template <typename IterableType>
    requires StringConvertibleIteratorType<IterableType, StringFormat>
    static BaseString toString(IterableType &&iterable)
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

    // std::format preprocessors
    template <HasStdFormatter Type>
    constexpr static Type toFormatValue(Type &&value)
    {
        return std::forward<Type>(value);
    }
    template <HasStringFormatToStringImpl<StringFormat> Type>
    requires (!HasStdFormatter<Type>)
    constexpr static String toFormatValue(Type &&value)
    {
        return toString(std::forward<Type>(value));
    }

private:
    StringFormat() = default;

#if USING_WIDE_UNICODE
#define STRING_PRINTF std::swprintf
#define STRING_VPRINTF std::vswprintf
#else // USING_WIDE_UNICODE
#define STRING_PRINTF std::snprintf
#define STRING_VPRINTF std::vsnprintf
#endif
    // && (Not necessary but nice to have this)passes the type as it is from the caller like r-values as
    // well, else r-values gets converted to l-values on this call
    template <typename... Args>
    DEBUG_INLINE static String stringPrintf(const TChar *fmt, Args &&...args)
    {
        int32 size = STRING_PRINTF(nullptr, 0, fmt, getChar<Args>(args)...);
        String fmted;
        // +1 size printf needs one more in buffer for null char
        fmted.resize(size + 1u);
        STRING_PRINTF(fmted.data(), size + 1, fmt, getChar<Args>(std::forward<Args>(args))...);
        // Resizing it back to original length
        fmted.resize(size);
        return fmted;
    }
    DEBUG_INLINE static String stringPrintf(const TChar *fmt, va_list args)
    {
        int32 size = STRING_VPRINTF(nullptr, 0, fmt, args);
        String fmted;
        // +1 size printf needs one more in buffer for null char
        fmted.resize(size + 1u);
        STRING_PRINTF(fmted.data(), size + 1, fmt, args);
        // Resizing it back to original length
        fmted.resize(size);
        return fmted;
    }
#undef STRING_PRINTF
#undef STRING_VPRINTF

public:
    template <typename FmtType, typename... Args>
    DEBUG_INLINE static String printf(FmtType &&fmt, Args &&...args)
    {
        return stringPrintf(getChar<FmtType>(std::forward<FmtType>(fmt)), toString<Args>(std::forward<Args>(args))...);
    }
    template <typename FmtType>
    DEBUG_INLINE static String printf(FmtType &&fmt, va_list args)
    {
        return stringPrintf(getChar<FmtType>(std::forward<FmtType>(fmt)), args);
    }
    template <StringLiteral Fmt, typename... Args>
    constexpr static String format(Args &&...args)
    {
        return std::format(Fmt.value, toFormatValue<Args>(std::forward<Args>(args))...);
    }
    template <typename FmtType, typename... Args>
    DEBUG_INLINE static String vFormat(FmtType &&fmt, Args &&...args)
    {
        return std::vformat(
            getChar<FmtType>(std::forward<FmtType>(fmt)),
            std::make_format_args<StdFormatContextType>(toString<Args>(std::forward<Args>(args))...)
        );
    }
};
