/*!
 * \file StringFormat.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2024
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
template <typename Type>
concept HasStdFormatter = requires(Type value, StdFormatContextType &context) {
    std::declval<typename StdFormatContextType::template formatter_type<std::remove_cvref_t<Type>>>().format(value, context);
};
#define STR_FORMAT(Fmt, ...) StringFormat::lFormat<TCHAR(Fmt)>(__VA_ARGS__)

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
    template <HasValidCStrMethod StrType>
    constexpr static const TChar *getChar(const StrType &value) noexcept
    {
        return static_cast<const TChar *>(value.c_str());
    }

    template <HasValidGetCharMethod StrType>
    requires (!HasValidCStrMethod<StrType>)
    constexpr static const TChar *getChar(const StrType &value) noexcept
    {
        return static_cast<const TChar *>(value.getChar());
    }

    template <std::convertible_to<const TChar *> StrType>
    constexpr static const TChar *getChar(StrType value) noexcept
    {
        return static_cast<const TChar *>(value);
    }

    /* Fall through getChar() for non string type */
    template <NonStringType Type>
    constexpr static Type getChar(Type &&value) noexcept
    {
        return std::forward<Type>(value);
    }

    /* Pass through for void * */
    template <typename Type>
    requires (std::is_void_v<UnderlyingType<Type>>)
    constexpr static const auto toString(Type value) noexcept
    {
        return value;
    }
    /* toString - To String templates */
    template <HasToStringMethod Type>
    FORCE_INLINE static String toString(const Type &value) noexcept
    {
        return value.toString();
    }

    /* All string and primitive(fundamental) type */
    template <StringOrFundamentalTypes Type>
    constexpr static Type toString(Type &&value) noexcept
    {
        return std::forward<Type>(value);
    }

    /* Only std::ostream << type exists */
    template <HasOStreamInsertOverrideMethod Type>
    requires (!std::is_void_v<UnderlyingType<Type>>)
    FORCE_INLINE static BaseString toString(const Type &value) noexcept
    {
        OStringStream stream;
        stream << value;
        return stream.str();
    }

    template <HasStringFormatToStringImpl<StringFormat> KeyType, HasStringFormatToStringImpl<StringFormat> ValueType>
    static BaseString toString(const std::pair<KeyType, ValueType> &pair) noexcept
    {
        OStringStream stream;
        stream << TCHAR("{ ") << toString(pair.first) << TCHAR(", ") << toString(pair.second) << TCHAR(" }");
        return stream.str();
    }

    template <StringConvertibleIteratorType<StringFormat> IterableType>
    static BaseString toString(IterableType &&iterable) noexcept
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
    constexpr static Type toFormatValue(Type &&value) noexcept
    {
        return std::forward<Type>(value);
    }
    template <HasStringFormatToStringImpl<StringFormat> Type>
    requires (!HasStdFormatter<Type>)
    constexpr static String toFormatValue(Type &&value) noexcept
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
    DEBUG_INLINE static String stringPrintf(const TChar *fmt, Args &&...args) noexcept
    {
        int32 size = STRING_PRINTF(nullptr, 0, fmt, getChar(args)...);
        String fmted;
        // +1 size printf needs one more in buffer for null char
        fmted.resize(size + 1u);
        STRING_PRINTF(fmted.data(), size + 1, fmt, getChar(args)...);
        // Resizing it back to original length
        fmted.resize(size);
        return fmted;
    }
    DEBUG_INLINE static String stringPrintf(const TChar *fmt, va_list args) noexcept
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

    /* This indirection needed as std::vformat's std::make_format_args requires only l-value references */
    template <typename... Args>
    DEBUG_INLINE static String stringVFormat(const TChar *fmt, Args &&...args) noexcept
    {
        return std::vformat(fmt, std::make_format_args<StdFormatContextType>(args...));
    }

public:
    /* Provided to work with libraries that uses Ansi or utf8 Char printf */
    template <typename FmtType, typename... Args>
    NODISCARD static auto charPrintf(FmtType &&fmt, Args &&...args) noexcept
    {
#if USING_WIDE_UNICODE
        int32 size = std::snprintf(nullptr, 0, fmt, args...);
        std::string fmted;
        // +1 size printf needs one more in buffer for null char
        fmted.resize(size + 1u);
        STRING_PRINTF(fmted.data(), size + 1, fmt, std::forward<Args>(args)...);
        // Resizing it back to original length
        fmted.resize(size);
        return fmted;
#else
        return printf(std::forward<FmtType>(fmt), std::forward<Args>(args)...);
#endif
    }

    template <typename FmtType, typename... Args>
    NODISCARD static String printf(FmtType &&fmt, Args &&...args) noexcept
    {
        return stringPrintf(getChar<FmtType>(fmt), toString<Args>(std::forward<Args>(args))...);
    }
    template <typename FmtType>
    NODISCARD static String printf(FmtType &&fmt, va_list args) noexcept
    {
        return stringPrintf(getChar<FmtType>(fmt), args);
    }
    template <StringLiteral Fmt, typename... Args>
    NODISCARD constexpr static String lFormat(Args &&...args) noexcept
    {
        return std::format(Fmt.value, toFormatValue<Args>(std::forward<Args>(args))...);
    }
    template <typename FmtType, typename... Args>
    NODISCARD static String vFormat(FmtType &&fmt, Args &&...args) noexcept
    {
        return stringVFormat(getChar<FmtType>(fmt), toFormatValue<Args>(std::forward<Args>(args))...);
    }
};
