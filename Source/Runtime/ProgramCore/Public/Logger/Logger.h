#pragma once
#include "Memory/SmartPointers.h"
#include "String/String.h"
#include "ProgramCoreExports.h"

#include <sstream>
#include <cstdio>

class GenericFile;

class PROGRAMCORE_EXPORT Logger 
{
private:
    Logger() = default;

    static std::ostringstream& loggerBuffer();
    static GenericFile* getLogFile();

    // getChar - String to AChar* and primitive types pass templates
    template<typename StrType>
    FORCE_INLINE static std::enable_if_t<std::is_convertible_v<decltype(std::declval<StrType>().c_str()), const AChar*>, const AChar*>
        getChar(const StrType& value)
    {
        return static_cast<const AChar*>(value.c_str());
    }

    template<typename StrType>
    FORCE_INLINE static std::enable_if_t<
        std::conjunction_v<
        std::is_convertible<decltype(std::declval<StrType>().getChar()), const AChar*>, std::negation<std::is_function<decltype(StrType::c_str)>>>, const AChar*>
        getChar(const StrType& value)
    {
        return static_cast<const AChar*>(value.getChar());
    }

    template<typename StrType>
    FORCE_INLINE static const std::enable_if_t<std::is_convertible_v<StrType, const AChar*>, const AChar*>
        getChar(StrType value)
    {
        return static_cast<const AChar*>(value);
    }

    template<typename StrType>
    FORCE_INLINE static const std::enable_if_t<std::negation_v<IsString<StrType>>, StrType>
        getChar(StrType value)
    {
        return value;
    }

    template<typename... Args>
    static String fmtString(const AChar* fmt, Args... args)
    {
        int32 size = std::snprintf(nullptr, 0, fmt, getChar<Args>(args)...);
        String fmted;
        fmted.resize(size + 1);
        std::snprintf(fmted.data(), size + 1, fmt, getChar<Args>(args)...);
        return fmted;
    }

    // toString - To String templates
    // If not a string type and toString method exists
    template<typename Type>
    FORCE_INLINE static std::enable_if_t<
        std::conjunction_v<std::negation<IsString<Type>>
        , std::is_compound<Type>
        , std::is_convertible<decltype(std::declval<Type>().toString()), String>>, String>
        toString(const Type& value)
    {
        return value.toString();
    }

    // Only std::ostream << type exists
    template<typename Type>
    FORCE_INLINE static std::enable_if_t<
        std::conjunction_v<std::negation<IsString<Type>>
        , std::is_compound<Type>
        , std::is_same<decltype(std::declval<std::ostream&>() << std::declval<Type>()), std::ostream&>>, std::string>
        toString(const Type& value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    // All string and primitive(fundamental) type
    template<typename Type>
    FORCE_INLINE static std::enable_if_t<std::disjunction_v<IsString<Type>, std::negation<std::is_compound<Type>>>, Type>
        toString(const Type& value)
    {
        return value;
    }

    static void debugInternal(const AChar* category, const String& message);
    static void logInternal(const AChar* category, const String& message);
    static void warnInternal(const AChar* category, const String& message);
    static void errorInternal(const AChar* category, const String& message);
public:
    template<typename CatType, typename FmtType, typename... Args>
    static void debug(const CatType category, const FmtType fmt, Args... args)
    {
        debugInternal(getChar(category), fmtString(getChar(fmt), toString<Args>(args)...));
    }

    template<typename CatType, typename FmtType, typename... Args>
    static void log(const CatType category, const FmtType fmt, Args... args)
    {
        logInternal(getChar(category), fmtString(getChar(fmt), toString<Args>(args)...));
    }

    template<typename CatType, typename FmtType, typename... Args>
    static void warn(const CatType category, const FmtType fmt, Args... args)
    {
        warnInternal(getChar(category), fmtString(getChar(fmt), toString<Args>(args)...));
    }

    template<typename CatType, typename FmtType, typename... Args>
    static void error(const CatType category, const FmtType fmt, Args... args)
    {
        errorInternal(getChar(category), fmtString(getChar(fmt), toString<Args>(args)...));
    }

    static void flushStream();
};