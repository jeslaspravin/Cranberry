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
#include "Types/Platform/PlatformAssertionErrors.h"

#include <regex>

String PropertyHelper::getValidSymbolName(const String& inValue)
{
    // Replace all Pointers as Ptr and References as Ref
    String postReplaceRefPtr = inValue.replaceAllCopy("*", "Ptr");
    postReplaceRefPtr.replaceAll("&", "Ref");

    // Now replace other invalid chars as _
    String output;
    output.resize(postReplaceRefPtr.length());

    static const std::regex matchPattern("^[0-9]{1}|[^a-zA-Z0-9_]{1}", std::regex_constants::ECMAScript);
    std::regex_replace(output.begin(), postReplaceRefPtr.cbegin(), postReplaceRefPtr.cend(), matchPattern, "_");

    return output;
}

bool PropertyHelper::isValidSymbolName(const String& inValue)
{
    static const std::regex matchPattern(VALID_SYMBOL_REGEX_PATTERN, std::regex_constants::ECMAScript);
    return std::regex_match(inValue, matchPattern);
}

bool PropertyHelper::isValidFunctionCall(const String& inValue)
{
    // Start with valid symbol then open and close braces followed by space or ;
    static const std::regex matchPattern(COMBINE(VALID_SYMBOL_REGEX_PATTERN, " *\\(.*\\)[ ;]*"), std::regex_constants::ECMAScript);
    return std::regex_match(inValue, matchPattern);
}
