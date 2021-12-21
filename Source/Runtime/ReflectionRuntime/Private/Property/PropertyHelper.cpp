#include "Property/PropertyHelper.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <regex>

String PropertyHelper::getValidSymbolName(const String& inValue)
{
    String output;
    output.resize(inValue.length());

    static const std::regex matchPattern("^[0-9]{1}|[^a-zA-Z0-9_]{1}", std::regex_constants::ECMAScript);
    std::regex_replace(output.begin(), inValue.cbegin(), inValue.cend(), matchPattern, "_");

    return output;
}
