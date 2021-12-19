#pragma once

#include "String/String.h"
#include "ProgramCoreExports.h"

#include <unordered_map>

template <typename T>
concept HasValidCStrMethod = requires(T&& val)
{
    { val.c_str() } -> std::convertible_to<const AChar*>;
};

template <typename T>
concept HasValidGetCharMethod = requires(T&& val)
{
    { val.getChar() } -> std::convertible_to<const AChar*>;
};

// If not a string type and toString method exists
template<typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept HasToStringMethod = NonStringType<Type> && std::is_compound_v<CleanType>
    && requires(Type && val)
    {
        { val.toString() } -> std::convertible_to<String>;
    };

template<typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept HasOStreamInsertOverrideMethod = NonStringType<Type> && std::is_compound_v<CleanType>
    && requires(Type && val)
    {
        { std::declval<std::ostream&>() << std::declval<Type>() } -> std::same_as<std::ostream&>;
    };

template<typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept StringOrFundamentalTypes = std::disjunction_v<IsString<Type>, std::is_fundamental<CleanType>>;

struct PROGRAMCORE_EXPORT FormatArg
{
    union ArgValue
    {
        String strVal;
        CoreTypesUnion fundamentalVals;

        ArgValue(CoreTypesUnion inFundamentalVals)
            : fundamentalVals(inFundamentalVals)
        {}
        template <typename StrType> requires StringType<StrType>
        ArgValue(StrType&& inStrVal)
            : strVal(std::forward<StrType>(inStrVal))
        {}
        ArgValue()
            : strVal()
        {}
        ~ArgValue()
        {}
    };
    ArgValue value;
    enum
    {
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
        AsString // Whatever other type that will be stored as string after toString conversion
    } type;

    FormatArg(bool argValue)    : type(Bool)    { value.fundamentalVals.boolVal = argValue; }
    FormatArg(uint8 argValue)   : type(UInt8)   { value.fundamentalVals.uInt8Val = argValue; }
    FormatArg(uint16 argValue)  : type(UInt16)  { value.fundamentalVals.uInt16Val = argValue; }
    FormatArg(uint32 argValue)  : type(UInt32)  { value.fundamentalVals.uInt32Val = argValue; }
    FormatArg(uint64 argValue)  : type(UInt64)  { value.fundamentalVals.uInt64Val = argValue; }
    FormatArg(int8 argValue)    : type(Int8)    { value.fundamentalVals.int8Val = argValue; }
    FormatArg(int16 argValue)   : type(Int16)   { value.fundamentalVals.int16Val = argValue; }
    FormatArg(int32 argValue)   : type(Int32)   { value.fundamentalVals.int32Val = argValue; }
    FormatArg(int64 argValue)   : type(Int64)   { value.fundamentalVals.int64Val = argValue; }
    FormatArg(float argValue)   : type(Float)   { value.fundamentalVals.floatVal = argValue; }
    FormatArg(double argValue)  : type(Double)  { value.fundamentalVals.doubleVal = argValue; }
    template <typename InType>
    FormatArg(InType&& argValue);

    FormatArg(const FormatArg& arg);
    FormatArg(FormatArg&& arg);
    FormatArg& operator=(const FormatArg& arg);
    FormatArg& operator=(FormatArg&& arg);

    String toString() const;
};

class StringFormat
{
public:
    // getChar - String to AChar* and primitive types pass templates
    template<typename StrType> requires HasValidCStrMethod<StrType>
    FORCE_INLINE CONST_EXPR static const AChar* getChar(const StrType& value)
    {
        return static_cast<const AChar*>(value.c_str());
    }

    template<typename StrType> requires HasValidGetCharMethod<StrType> && (!HasValidCStrMethod<StrType>)
    FORCE_INLINE CONST_EXPR static const AChar* getChar(const StrType& value)
    {
        return static_cast<const AChar*>(value.getChar());
    }

    template<typename StrType> requires std::convertible_to<StrType, const AChar*>
    FORCE_INLINE CONST_EXPR static const AChar* getChar(StrType&& value)
    {
        return static_cast<const AChar*>(value);
    }

    template<typename StrType> requires NonStringType<StrType>
    FORCE_INLINE CONST_EXPR static StrType getChar(StrType&& value)
    {
        return std::forward<StrType>(value);
    }    

    // toString - To String templates
    template<typename Type> requires HasToStringMethod<Type>
    FORCE_INLINE CONST_EXPR static String toString(const Type& value)
    {
        return value.toString();
    }

    // Only std::ostream << type exists
    template<typename Type> requires HasOStreamInsertOverrideMethod<Type>
    FORCE_INLINE CONST_EXPR static std::string toString(const Type& value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    // All string and primitive(fundamental) type
    template<typename Type> requires StringOrFundamentalTypes<Type>
    FORCE_INLINE CONST_EXPR static Type toString(Type&& value)
    {
        return std::forward<Type>(value);
    }

private:
    StringFormat() = default;

    // && (Not necessary but nice to have this)passes the type as it is from the caller like r-values as well, else r-values gets converted to l-values on this call
    template<typename... Args>
    DEBUG_INLINE CONST_EXPR static String fmtString(const AChar* fmt, Args&&... args)
    {
        int32 size = std::snprintf(nullptr, 0, fmt, getChar<Args>(std::forward<Args>(args))...);
        String fmted;
        // +1 size printf needs one more in buffer for null char
        fmted.resize(size + 1);
        std::snprintf(fmted.data(), size + 1, fmt, getChar<Args>(std::forward<Args>(args))...);
        // Resizing it back to original length
        fmted.resize(size);
        return fmted;
    }

public:

    template<typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static String format(FmtType&& fmt, Args&&... args)
    {
        return fmtString(getChar<FmtType>(std::forward<FmtType>(fmt)), toString<Args>(std::forward<Args>(args))...);
    }

    PROGRAMCORE_EXPORT static String formatMustache(const String& fmt, const std::unordered_map<String, FormatArg>& formatArgs);
};

template <typename InType>
FormatArg::FormatArg(InType&& argValue)
    : value(StringFormat::toString<InType>(std::forward<InType>(argValue)))
    , type(FormatArg::AsString)
{}
