/*!
 * \file PropertyHelper.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Property/PropertyHelper.h"
#include "String/StringRegex.h"
#include "Types/Platform/PlatformAssertionErrors.h"

String PropertyHelper::getValidSymbolName(const String &inValue)
{
    // Replace all Pointers as Ptr and References as Ref
    String postReplaceRefPtr = inValue.replaceAllCopy(TCHAR("*"), TCHAR("Ptr"));
    postReplaceRefPtr.replaceAll(TCHAR("&"), TCHAR("Ref"));

    // Now replace other invalid chars as _
    String output;
    output.resize(postReplaceRefPtr.length());

    static const StringRegex matchPattern(TCHAR("^[0-9]{1}|[^a-zA-Z0-9_]{1}"), std::regex_constants::ECMAScript);
    std::regex_replace(output.begin(), postReplaceRefPtr.cbegin(), postReplaceRefPtr.cend(), matchPattern, TCHAR("_"));

    return output;
}

bool PropertyHelper::isValidSymbolName(const String &inValue)
{
    static const StringRegex matchPattern(VALID_SYMBOL_REGEX_PATTERN, std::regex_constants::ECMAScript);
    return std::regex_match(inValue, matchPattern);
}

bool PropertyHelper::isValidFunctionCall(const String &inValue)
{
    // Start with valid symbol then open and close braces followed by space or ;
    static const StringRegex matchPattern(COMBINE(VALID_SYMBOL_REGEX_PATTERN, " *\\(.*\\)[ ;]*"), std::regex_constants::ECMAScript);
    return std::regex_match(inValue, matchPattern);
}

bool PropertyHelper::isChildOf(const ClassProperty *childClassProp, const ClassProperty *parentClassProp)
{
    if (!childClassProp || !parentClassProp)
    {
        alertIf(childClassProp && parentClassProp, "Null class properties are not valid input for isChildOf function");
        return false;
    }

    std::vector<const ClassProperty *> checkClasses;
    checkClasses.emplace_back(childClassProp);
    while (!checkClasses.empty())
    {
        std::vector<const ClassProperty *> newCheckClasses;
        for (const ClassProperty *clazz : checkClasses)
        {
            // If matched property found return, else keep on adding base classes until a match
            // is found or empty
            if (clazz == parentClassProp)
            {
                return true;
            }
            newCheckClasses.insert(newCheckClasses.end(), clazz->baseClasses.cbegin(), clazz->baseClasses.cend());
        }
        checkClasses = std::move(newCheckClasses);
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
        alertIf(childClassProp && interfaceType, "Null class properties are not valid input for implementsInterface function");
        return nullptr;
    }

    for (const InterfaceInfo &interfaceInfo : childClassProp->interfaces)
    {
        if (interfaceInfo.interfaceTypeInfo == interfaceType)
        {
            return &interfaceInfo;
        }
    }

    for (const ClassProperty *baseClazz : childClassProp->baseClasses)
    {
        if (const InterfaceInfo *interfaceInfo = getMatchingInterfaceInfo(baseClazz, interfaceType))
        {
            return interfaceInfo;
        }
    }
    return nullptr;
}

bool PropertyHelper::implementsInterface(const ClassProperty *childClassProp, const ReflectTypeInfo *interfaceType)
{
    return !!getMatchingInterfaceInfo(childClassProp, interfaceType);
}
