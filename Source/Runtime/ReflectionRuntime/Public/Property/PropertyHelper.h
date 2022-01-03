#pragma once
#include "String/String.h"
#include "ReflectionRuntimeExports.h"

#define VALID_SYMBOL_REGEX_PATTERN "^[a-zA-Z_]{1}[a-zA-Z0-9_]*"
class REFLECTIONRUNTIME_EXPORT PropertyHelper
{
private:
    PropertyHelper() = default;
public:
    // Returns valid name that be used as name for function or class or other user symbol in C++. 
    // Does not handles special key words in C++ so caller must ensure that resulting string does not leads to keyword
    // Replaces any none allowed characters in symbol name to underscore
    static String getValidSymbolName(const String& inValue);
    static bool isValidSymbolName(const String& inValue);
    static bool isValidFunctionCall(const String& inValue);
};