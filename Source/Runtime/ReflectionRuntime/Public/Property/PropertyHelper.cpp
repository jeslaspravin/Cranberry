/*!
 * \file PropertyHelper.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Property/PropertyHelper.h"
#include "String/StringRegex.h"
#include "Types/Platform/PlatformAssertionErrors.h"

String PropertyHelper::getValidSymbolName(StringView inValue)
{
    String postReplaceRefPtr = inValue;
    // Replace all Pointers as Ptr and References as Ref
    postReplaceRefPtr.replaceAll(TCHAR("*"), TCHAR("Ptr"));
    postReplaceRefPtr.replaceAll(TCHAR("&"), TCHAR("Ref"));

    // Now replace other invalid chars as _
    String output;
    output.resize(postReplaceRefPtr.length());

    static const StringRegex matchPattern(TCHAR("^[0-9]{1}|[^a-zA-Z0-9_]{1}"), std::regex_constants::ECMAScript);
    std::regex_replace(output.begin(), postReplaceRefPtr.cbegin(), postReplaceRefPtr.cend(), matchPattern, TCHAR("_"));

    return output;
}

bool PropertyHelper::isValidSymbolName(StringView inValue)
{
    static const StringRegex matchPattern(VALID_SYMBOL_REGEX_PATTERN, std::regex_constants::ECMAScript);
    return std::regex_match(inValue.cbegin(), inValue.cend(), matchPattern);
}

bool PropertyHelper::isValidFunctionCall(StringView inValue)
{
    // Start with valid symbol then open and close bracets followed by space or ;
    // \s - any white space incl \n, \S all non white space chars
    static const StringRegex matchPattern(COMBINE(VALID_SYMBOL_REGEX_PATTERN, "\\s*\\([\\s\\S]*\\)[ ;]?"), std::regex_constants::ECMAScript);
    return std::regex_match(inValue.cbegin(), inValue.cend(), matchPattern);
}

bool PropertyHelper::isValidConstructionCall(StringView inValue)
{
    // Start with valid symbol then open and close braces followed by space or ;
    // \s - any white space incl \n, \S all non white space chars
    static const StringRegex matchPattern(COMBINE(VALID_SYMBOL_REGEX_PATTERN, "\\s*[({][\\s\\S]*[)}][ ;]?"), std::regex_constants::ECMAScript);
    return std::regex_match(inValue.cbegin(), inValue.cend(), matchPattern);
}

bool PropertyHelper::isChildOf(const ClassProperty *childClassProp, const ClassProperty *parentClassProp)
{
    if (!childClassProp || !parentClassProp)
    {
        alertAlwaysf(childClassProp && parentClassProp, "Null class properties are not valid input for isChildOf function");
        return false;
    }

    CBE_PROFILER_SCOPE("IsChildOf");

    const ClassProperty *checkProp = childClassProp;
    while (checkProp)
    {
        if (checkProp == parentClassProp)
        {
            return true;
        }
        checkProp = checkProp->baseClass;
    }
    return false;
}

bool PropertyHelper::isStruct(const ClassProperty *classProp)
{
    return IReflectionRuntimeModule::get()->getStructType(classProp->typeInfo) == classProp;
}

const InterfaceInfo *PropertyHelper::getMatchingInterfaceInfo(const ClassProperty *childClassProp, const ReflectTypeInfo *interfaceType)
{
    if (!childClassProp || !interfaceType)
    {
        alertAlwaysf(childClassProp && interfaceType, "Null class properties are not valid input for implementsInterface function");
        return nullptr;
    }

    CBE_PROFILER_SCOPE("GetMatchingInterface");

    for (const InterfaceInfo &interfaceInfo : childClassProp->interfaces)
    {
        if (interfaceInfo.interfaceTypeInfo == interfaceType)
        {
            return &interfaceInfo;
        }
    }

    if (const InterfaceInfo *interfaceInfo = getMatchingInterfaceInfo(childClassProp->baseClass, interfaceType))
    {
        return interfaceInfo;
    }
    return nullptr;
}

bool PropertyHelper::implementsInterface(const ClassProperty *childClassProp, const ReflectTypeInfo *interfaceType)
{
    return !!getMatchingInterfaceInfo(childClassProp, interfaceType);
}
