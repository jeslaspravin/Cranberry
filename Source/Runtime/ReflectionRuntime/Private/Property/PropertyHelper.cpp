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
