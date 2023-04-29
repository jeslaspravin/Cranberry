/*!
 * \file ParserHelper.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Property/Property.h"
#include "String/String.h"

#include <clang-c/Index.h>

struct ReflectedTypeItem;
struct SourceGeneratorContext;

struct ParsedMetaData
{
    String metaType;
    String ctorArgs;
};

class ParserHelper
{
private:
    ParserHelper() = default;

    static bool commonTypeValidity(CXType clangType);
    // Returns false if any errors
    static bool parseMeta(std::vector<String> &possibleFlags, std::vector<ParsedMetaData> &metaData, const String &annotatedStr);

public:
    static NODISCARD bool shouldReflectHeader(const String &headerFilePath);
    static NODISCARD bool shouldReflectHeader(const String &headerFilePath, const std::vector<StringView> &lines);

    // Returns false if any errors
    static NODISCARD bool parseClassMeta(
        std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
    );
    static NODISCARD bool parseFieldMeta(
        std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
    );
    static NODISCARD bool parseFunctionMeta(
        std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
    );
    static NODISCARD bool parseEnumMeta(
        std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
    );

    // Gets type name without any const attached to it
    // Cursor if available will be used over type to find type-ref from it
    static NODISCARD String getNonConstTypeName(CXType clangType, CXCursor typeRefCursor);
    // Cursor must be one that has possible type reference
    // Type obtained from this will not have reference or pointer, but have const-ness
    static NODISCARD CXType getTypeReferred(CXType clangType, CXCursor typeRefCursor);
    static NODISCARD CXCursor getTypeRefInCursor(CXCursor cursor);
    static NODISCARD String getCursorTypeName(CXCursor cursor);
    // Just returns canonical type name. No checks will be made
    static NODISCARD String getCursorCanonicalTypeName(CXCursor cursor);
    static NODISCARD String accessSpecifierName(CXCursor cursor);

    static NODISCARD bool isBuiltinType(CXType clangType);
    /*
     * Returns true if type is one of specialized types like Vector3, Rotation ...
     * Cursor must be one that has possible type reference
     */
    static NODISCARD bool isSpecializedType(CXType clangType, CXCursor typeRefCursor);
    /*
     * Returns true if type is one of custom types like std::map,std::vector ...
     * Cursor must be one that has possible type reference
     */
    static NODISCARD bool isCustomType(CXType clangType, CXCursor typeRefCursor, SourceGeneratorContext *srcGenContext);
    static NODISCARD bool isReflectedDecl(CXCursor declCursor);
    static NODISCARD bool isReflectedDecl(CXCursor declCursor, SourceGeneratorContext *srcGenContext);
    /*
     * Checks if class/struct is annotated and also has generated codes
     */
    static NODISCARD bool isReflectedClass(CXCursor declCursor);
    static NODISCARD bool isReflectedClass(CXCursor declCursor, SourceGeneratorContext *srcGenContext);
    static NODISCARD bool hasOverridenCtorPolicy(CXCursor declCursor);
    static NODISCARD CXCursor getGeneratedCodeCursor(CXCursor declCursor);
    static NODISCARD String getCursorMetaString(CXCursor cursor);

    /**
     * If a class is interface. Interface will not have reflection generated. So only annotation is expected generated codes must not be present
     * if class is marked interface
     */
    static NODISCARD bool isInterfaceClass(CXCursor declCursor);
    // Validates if an interface class hierarchy is all interface and valid.
    static NODISCARD bool getInterfaceHierarchy(std::vector<CXCursor> &allInterfaces, CXCursor declCursor);

    static NODISCARD bool getMapElementTypes(CXType &outKeyType, CXType &outValueType, CXType mapType, CXCursor mapTypeRefCursor);
    static NODISCARD bool getPairElementTypes(CXType &outKeyType, CXType &outValueType, CXType pairType, CXCursor pairTypeRefCursor);
    static NODISCARD bool getContainerElementType(CXType &outType, CXType containerType, CXCursor typeRefCursor);

    static NODISCARD bool isValidFuncParamType(CXType clangType, CXCursor paramCursor);
    static NODISCARD bool isValidFuncReturnType(CXType clangType);
    static NODISCARD bool isValidFunction(CXCursor funcCursor);
    static NODISCARD bool isValidFieldType(CXType clangType, CXCursor fieldCursor, SourceGeneratorContext *srcGenContext);
};