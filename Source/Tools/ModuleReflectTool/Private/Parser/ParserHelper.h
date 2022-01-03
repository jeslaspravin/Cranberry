#pragma once

#include "String/String.h"
#include "Property/Property.h"

#include <clang-c/Index.h>


class ParserHelper
{
private:
    ParserHelper() = default;

    static bool commonTypeValidity(CXType clangType);
    static std::vector<String> parseMeta(std::vector<String>& metaData, const String& annotatedStr);
public:
    static bool shouldReflectHeader(const String& headerFilePath);
    static void parseClassMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr);
    static void parseFieldMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr);
    static void parseFunctionMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr);
    static void parseEnumMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr);

    // Gets type name without any const attached to it
    // Cursor if available will be used over type to find type-ref from it
    static String getNonConstTypeName(CXType clangType, CXCursor typeRefCursor);
    // Cursor must be one that has possible type reference
    // Type obtained from this will not have reference or pointer, but have const-ness
    static CXType getTypeReferred(CXType clangType, CXCursor typeRefCursor);
    static CXCursor getTypeRefInCursor(CXCursor cursor);
    static String getCursorTypeName(CXCursor cursor);
    static String accessSpecifierName(CXCursor cursor);

    static bool isBuiltinType(CXType clangType);
    /*
    * Returns true if type is one of specialized types like Vector3D, Rotation ...
    * Cursor must be one that has possible type reference
    */
    static bool isSpecializedType(CXType clangType, CXCursor typeRefCursor);
    /*
    * Returns true if type is one of custom types like std::map,std::vector ...
    * Cursor must be one that has possible type reference
    */
    static bool isCustomType(CXType clangType, CXCursor typeRefCursor);
    static bool isReflectedDecl(CXCursor declCursor);
    /*
    * Checks if class/struct is annotated and also has generated codes
    */
    static bool isReflectedClass(CXCursor declCursor);
    static CXCursor getGeneratedCodeCursor(CXCursor declCursor);
    static String getCursorMetaString(CXCursor cursor);

    static bool getMapElementTypes(CXType& outKeyType, CXType& outValueType, CXType mapType, CXCursor mapTypeRefCursor);
    static bool getPairElementTypes(CXType& outKeyType, CXType& outValueType, CXType pairType, CXCursor pairTypeRefCursor);
    static bool getContainerElementType(CXType& outType, CXType containerType, CXCursor typeRefCursor);

    static bool isValidFuncParamType(CXType clangType, CXCursor paramCursor);
    static bool isValidFuncReturnType(CXType clangType);
    static bool isValidFunction(CXCursor funcCursor);
    static bool isValidFieldType(CXType clangType, CXCursor fieldCursor);
};