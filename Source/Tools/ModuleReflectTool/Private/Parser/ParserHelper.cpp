/*!
 * \file ParserHelper.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Parser/ParserHelper.h"
#include "Parser/ClangWrappers.h"
#include "ReflectionMacros.h"
#include "String/StringRegex.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PathFunctions.h"


String ParserHelper::getNonConstTypeName(CXType clangType, CXCursor typeRefCursor)
{
    CXType innerType = getTypeReferred(clangType, typeRefCursor);
    fatalAssert(innerType.kind != CXType_Invalid, "%s() : Type retrieval must not fail here! check the input", __func__);

    String typeName = CXStringWrapper(clang_getTypeSpelling(innerType)).toString();
    if (clang_isConstQualifiedType(innerType))
    {
        // Offset by "const " 6 char
        typeName = String(typeName.cbegin() + 6, typeName.cend());
    }
    return typeName;
}

CXType ParserHelper::getTypeReferred(CXType clangType, CXCursor typeRefCursor)
{
    CXCursor innerTypeCursor = getTypeRefInCursor(typeRefCursor);
    // If template ref then it is not possible to get the type name from cursor alone so use type for it
    if (!(clang_Cursor_isNull(innerTypeCursor) || clang_getCursorKind(innerTypeCursor) == CXCursor_TemplateRef))
    {
        // We do not have to recurse in cursor derived referred type as it is based on lexical cursor and TypeRef accurately mean referred type
        return clang_getCursorType(innerTypeCursor);
    }

    CXType innerType = clang_getPointeeType(clangType);
    switch (clangType.kind)
    {
    case CXType_RValueReference:
    case CXType_LValueReference:
    case CXType_Pointer:
        return getTypeReferred(innerType, innerTypeCursor);
        break;
    default:
        break;
    }
    return clangType;
}

CXCursor ParserHelper::getTypeRefInCursor(CXCursor cursor)
{
    CXCursor innerTypeCursor = clang_getNullCursor();
    if (!clang_Cursor_isNull(cursor))
    {
        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CXCursorKind cursorKind = clang_getCursorKind(c);
                if (cursorKind == CXCursor_TypeRef || cursorKind == CXCursor_TemplateRef)
                {
                    *(CXCursor*)(clientData) = c;
                    return CXChildVisit_Break;
                }
                return CXChildVisit_Continue;
            }
        , &innerTypeCursor);
    }
    return innerTypeCursor;
}

String ParserHelper::getCursorTypeName(CXCursor cursor)
{
    if (clang_Cursor_isNull(cursor))
    {
        return {};
    }
    CXType type = clang_getCursorType(cursor);
    CXType canonicalType = clang_getCanonicalType(type);
    String typeName = CXStringWrapper(clang_getTypeSpelling(type)).toString();
    String paramName = CXStringWrapper(clang_getCursorSpelling(cursor)).toString();
    // If not POD then we need to check canonical typename for generation
    if (!ParserHelper::isBuiltinType(ParserHelper::getTypeReferred(type, cursor)))
    {
        // Why getting from canonical type? Because it gives name with all the scopes prefixed. We do not have to handle parent namespace or types
        // Even inner types of templates are prefixed properly
        typeName = CXStringWrapper(clang_getTypeSpelling(canonicalType)).toString();
    }
    return typeName;
}

String ParserHelper::accessSpecifierName(CXCursor cursor)
{
    String access;
    CX_CXXAccessSpecifier currentScopeAccess = clang_getCXXAccessSpecifier(cursor);
    switch (currentScopeAccess)
    {
    case CX_CXXPublic:
        access = TCHAR("Public");
        break;
    case CX_CXXProtected:
        access = TCHAR("Protected");
        break;
    case CX_CXXPrivate:
        access = TCHAR("Private");
        break;
    case CX_CXXInvalidAccessSpecifier:
    default:
        access = TCHAR("Public");
        break;
    }
    return access;
}

bool ParserHelper::isBuiltinType(CXType clangType)
{
    clangType = clang_getCanonicalType(clangType);
    return clangType.kind >= CXType_FirstBuiltin && clangType.kind <= CXType_LastBuiltin;
}

bool ParserHelper::shouldReflectHeader(const String& headerFilePath)
{
    String headerFileContent;
    if (!FileHelper::readString(headerFileContent, headerFilePath))
    {
        LOG_ERROR("ParserHelper", "%s() : Cannot open header file(%s) to read", __func__, headerFilePath);
        return false;
    }

    // First match "#include *[<\"]{1}.*\\.gen\\.h[>\"]{1}" checks if generated header file is included
    // Second match ".*META_ANNOTATE.*\\(.*\\)[ \t]*.*" checks if the there is a meta annotated field or declaration
    // Third match "[ \t]*GENERATED_CODES\\(\\)" match if there is a line with GENERATED_CODES() macro expansion
    // If anyone of above matches then we consider that header for reflection compiling, Further validations will be done there
    static const StringRegex searchPattern(TCHAR("(#include *[<\"]{1}.*\\.gen\\.h[>\"]{1}|.*META_ANNOTATE.*\\(.*\\)[ \t]*.*|[ \t]*GENERATED_CODES\\(\\))"), std::regex_constants::ECMAScript);

    bool bGenReflection = false;
    std::vector<StringView> lines = headerFileContent.splitLines();
    for (const StringView& codeLine : lines)
    {
        if (std::regex_match(codeLine.cbegin(), codeLine.cend(), searchPattern))
        {
            bGenReflection = true;
            break;
        }
    }
    if (bGenReflection)
    {
        String checkHeader(PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(headerFilePath)) + TCHAR(".gen.h"));
        static const StringRegex includePattern(TCHAR("#include *[<\"]{1}(.*)[>\"]{1}"), std::regex_constants::ECMAScript);
        uint32 genInclLine = std::numeric_limits<uint32>::max();
        uint32 lineNo = 0;
        for (const StringView& codeLine : lines)
        {
            std::match_results<decltype(codeLine.cbegin())> matchResult;
            if (std::regex_match(codeLine.cbegin(), codeLine.cend(), matchResult, includePattern))
            {
                String inclHeader(matchResult[matchResult.size() - 1].str());
                // If included gen found mark that line
                if (inclHeader.endsWith(checkHeader))
                {
                    genInclLine = lineNo;
                }
                else if(lineNo > genInclLine) // Not gen header but include after gen header print error and skip this header file
                {
                    LOG_ERROR("ParserHelper", "%s() : Generated header include(%s:%d) must be last include of this header file %s"
                        , __func__, checkHeader, genInclLine, headerFilePath);
                    bGenReflection = false;
                    break;
                }
            }
            lineNo++;
        }
    }
    return bGenReflection;
}

#define CHECK_TYPE_FIRST(TypeName) (TCHAR(#TypeName) == checkTypeName)
#define CHECK_TYPE(TypeName) || (TCHAR(#TypeName) == checkTypeName)
bool ParserHelper::isSpecializedType(CXType clangType, CXCursor typeRefCursor)
{
    String checkTypeName = getNonConstTypeName(clangType, typeRefCursor);
    return FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(CHECK_TYPE_FIRST, CHECK_TYPE, CHECK_TYPE);
}
#undef CHECK_TYPE
#undef CHECK_TYPE_FIRST

#define CHECK_TYPE_FIRST(TypeName) (checkTypeName.startsWith(TCHAR(#TypeName)))
#define CHECK_TYPE(TypeName) || (checkTypeName.startsWith(TCHAR(#TypeName)))
bool ParserHelper::isCustomType(CXType clangType, CXCursor typeRefCursor)
{
    String checkTypeName = getNonConstTypeName(clangType, typeRefCursor);
    return FOR_EACH_CUSTOM_TYPES_UNIQUE_FIRST_LAST(CHECK_TYPE_FIRST, CHECK_TYPE, CHECK_TYPE);
}
#undef CHECK_TYPE
#undef CHECK_TYPE_FIRST

bool ParserHelper::isReflectedDecl(CXCursor declCursor)
{
    if (!clang_isDeclaration(clang_getCursorKind(declCursor)))
    {
        return false;
    }

    bool bHasAnnotation = false;
    clang_visitChildren(declCursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            if (cursorKind == CXCursor_AnnotateAttr)
            {
                // If reflected then we must have annotated
                *(bool*)(clientData) = true;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Continue;
        }
    , &bHasAnnotation);

    return bHasAnnotation;
}

bool ParserHelper::isReflectedClass(CXCursor declCursor)
{
    if (!clang_isDeclaration(clang_getCursorKind(declCursor)))
    {
        return false;
    }

    bool bHasAnnotationAndGenCode[] = { false, false };
    clang_visitChildren(declCursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            bool* bValid = (bool*)(clientData);
            CXCursorKind cursorKind = clang_getCursorKind(c);
            if (cursorKind == CXCursor_AnnotateAttr)
            {
                // If reflected then we must have annotated
                bValid[0] = true;
            }
            // If generated type alias/typedef decl is present
            else if (cursorKind == CXCursor_TypeAliasDecl || cursorKind == CXCursor_TypedefDecl)
            {
                bValid[1] = bValid[1] || (CXStringWrapper(clang_getCursorSpelling(c)).toString() == TCHAR(MACRO_TO_STRING(GENERATED_CODES_ALIAS)));
            }
            return CXChildVisit_Continue;
        }
    , bHasAnnotationAndGenCode);

    return bHasAnnotationAndGenCode[0] && bHasAnnotationAndGenCode[1];
}

CXCursor ParserHelper::getGeneratedCodeCursor(CXCursor declCursor)
{
    CXCursor generatedCodeCursor = clang_getNullCursor();
    if (!clang_isDeclaration(clang_getCursorKind(declCursor)))
    {
        return generatedCodeCursor;
    }

    clang_visitChildren(declCursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            // If generated type alias/typedef decl is present
            if ((cursorKind == CXCursor_TypeAliasDecl || cursorKind == CXCursor_TypedefDecl) 
                && (CXStringWrapper(clang_getCursorSpelling(c)).toString() == TCHAR(MACRO_TO_STRING(GENERATED_CODES_ALIAS))))
            {
                *(CXCursor*)(clientData) = c;
            }
            return CXChildVisit_Continue;
        }
    , &generatedCodeCursor);

    return generatedCodeCursor;
}

String ParserHelper::getCursorMetaString(CXCursor cursor)
{
    String metaStr;
    clang_visitChildren(cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            if (cursorKind == CXCursor_AnnotateAttr)
            {
                *(String*)(clientData) = CXStringWrapper(clang_getCursorSpelling(c)).toString();
                return CXChildVisit_Break;
            }
            return CXChildVisit_Continue;
        }
    , & metaStr);

    return metaStr;
}

bool ParserHelper::getMapElementTypes(CXType& outKeyType, CXType& outValueType, CXType mapType, CXCursor mapTypeRefCursor)
{
    String mapName = getNonConstTypeName(mapType, mapTypeRefCursor);
    CXType referredType = getTypeReferred(mapType, mapTypeRefCursor);
    
    if (!(mapName.startsWith(TCHAR("std::map")) || mapName.startsWith(TCHAR("std::unordered_map"))))
    {
        return false;
    }

    int32 templatesCount = clang_Type_getNumTemplateArguments(referredType);
    fatalAssert(templatesCount >= 2, "%s() : Template %d count must be atleast 2 for type %s", __func__, templatesCount, mapName);

    outKeyType = clang_Type_getTemplateArgumentAsType(referredType, 0);
    outValueType = clang_Type_getTemplateArgumentAsType(referredType, 1);

    return outKeyType.kind != CXType_Invalid && outValueType.kind != CXType_Invalid;
}

bool ParserHelper::getPairElementTypes(CXType& outKeyType, CXType& outValueType, CXType pairType, CXCursor pairTypeRefCursor)
{
    String pairName = getNonConstTypeName(pairType, pairTypeRefCursor);
    CXType referredType = getTypeReferred(pairType, pairTypeRefCursor);

    if (!pairName.startsWith(TCHAR("std::pair")))
    {
        return false;
    }

    int32 templatesCount = clang_Type_getNumTemplateArguments(referredType);
    fatalAssert(templatesCount >= 2, "%s() : Template %d count must be atleast 2 for type %s", __func__, templatesCount, pairName);

    outKeyType = clang_Type_getTemplateArgumentAsType(referredType, 0);
    outValueType = clang_Type_getTemplateArgumentAsType(referredType, 1);
    return outKeyType.kind != CXType_Invalid && outValueType.kind != CXType_Invalid;
}

bool ParserHelper::getContainerElementType(CXType& outType, CXType containerType, CXCursor typeRefCursor)
{
    String typeName = getNonConstTypeName(containerType, typeRefCursor);
    CXType referredType = getTypeReferred(containerType, typeRefCursor);

    if (!(typeName.startsWith(TCHAR("std::set")) || typeName.startsWith(TCHAR("std::unordered_set")) || typeName.startsWith(TCHAR("std::vector"))))
    {
        return false;
    }

    int32 templatesCount = clang_Type_getNumTemplateArguments(referredType);
    fatalAssert(templatesCount >= 1, "%s() : Template %d count must be atleast 1 for type %s", __func__, templatesCount, typeName);

    outType = clang_Type_getTemplateArgumentAsType(referredType, 0);
    return outType.kind != CXType_Invalid;
}

bool ParserHelper::commonTypeValidity(CXType clangType)
{
    CXType innerType = (clangType.kind == CXType_Vector) ? clang_getElementType(clangType) : clang_getPointeeType(clangType);
    bool bIsValid = ParserHelper::isBuiltinType(clangType);
    switch (clangType.kind)
    {
    case CXType_RValueReference:
    case CXType_LValueReference:
        // Reference to ref is not valid
        bIsValid = (innerType.kind != CXType_LValueReference && innerType.kind != CXType_RValueReference)
            && commonTypeValidity(innerType);
        break;
    case CXType_Pointer:
        // Make sure that this is not pointer chain or pointer to ref which is not valid in language
        bIsValid = (innerType.kind != CXType_Pointer && innerType.kind != CXType_LValueReference && innerType.kind != CXType_RValueReference)
            && commonTypeValidity(innerType);
        break;
    case CXType_Vector:
    {
        bIsValid = (innerType.kind != CXType_LValueReference && innerType.kind != CXType_RValueReference)
            && commonTypeValidity(innerType);
        break;
    }
    case CXType_Record:
    case CXType_Enum:
    {
        // If record type either class or struct or template defs, However we do not have to check that here for common type
        bIsValid = true;
        break;
    }
    case CXType_Elaborated:
    case CXType_Typedef:
    {
        bIsValid = commonTypeValidity(clang_getCanonicalType(clangType));
    }
    case CXType_ConstantArray:
    case CXType_IncompleteArray:
    case CXType_DependentSizedArray:
    case CXType_VariableArray:
    default:
        break;
    }
    return bIsValid;
}

bool ParserHelper::isValidFuncParamType(CXType clangType, CXCursor paramCursor)
{
    // bool bIsValid = commonTypeValidity(clangType);
    // Right now we do not have any validation other than common for param type
    return commonTypeValidity(clangType);
}

bool ParserHelper::isValidFuncReturnType(CXType clangType)
{
    // bool bIsValid = commonTypeValidity(clangType);
    // Right now we do not have any validation other than common for return type
    return commonTypeValidity(clangType);
}

bool ParserHelper::isValidFunction(CXCursor funcCursor)
{
    CXStringRef functionName(new CXStringWrapper(clang_getCursorSpelling(funcCursor)));
    if (!(clang_getCursorKind(funcCursor) == CXCursor_FunctionDecl 
        || clang_getCursorKind(funcCursor) == CXCursor_Constructor 
        || clang_getCursorKind(funcCursor) == CXCursor_CXXMethod))
    {
        LOG_ERROR("ParserHelper", "%s() : Function %s is not a function declaration", __func__, functionName);
        return false;
    }
    CXRefQualifierKind methodCalledRefKind = clang_Type_getCXXRefQualifier(clang_getCursorType(funcCursor));
    if (methodCalledRefKind != CXRefQualifier_None)
    {
        LOG_ERROR("ParserHelper", "%s() : Reference typed only function(%s) is not supported in reflection", __func__, functionName);
        return false;
    }

    CXType funcRetType = clang_getCursorResultType(funcCursor);
    if (!isValidFuncReturnType(funcRetType))
    {
        LOG("ParserHelper", "%s ERROR %s() : Function %s return type %s is not valid", clang_getCursorLocation(funcCursor)
            , __func__, functionName, clang_getTypeSpelling(funcRetType));
        return false;
    }

    int32 paramsCount = clang_Cursor_getNumArguments(funcCursor);
    for (uint32 i = 0; i < paramsCount; ++i)
    {
        CXCursor paramCursor = clang_Cursor_getArgument(funcCursor, i);
        CXType paramType = clang_getCursorType(paramCursor);

        if (!isValidFuncParamType(paramType, paramCursor))
        {
            LOG("ParserHelper", "%s ERROR %s() : Function %s param %s at %d is not valid type %s", clang_getCursorLocation(paramCursor)
                , __func__, functionName, clang_getCursorSpelling(paramCursor), i, clang_getTypeSpelling(paramType));
            return false;
        }
    }
    return true;
}

bool ParserHelper::isValidFieldType(CXType clangType, CXCursor fieldCursor)
{
    CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(fieldCursor)));
    CXStringRef typeName(new CXStringWrapper(clang_getTypeSpelling(clangType)));
    bool bIsValid = commonTypeValidity(clangType);
    // Only pointer field can hold const type otherwise field type cannot be const
    bIsValid = bIsValid && (!clang_isConstQualifiedType(getTypeReferred(clangType, fieldCursor)) || clangType.kind == CXType_Pointer);
    if (bIsValid)
    {
        switch (clangType.kind)
        {
        case CXType_RValueReference:
        case CXType_LValueReference:
            // Reference is not valid for field
            bIsValid = false;
            LOG_ERROR("ParserHelper", "%s() : Reference type[%s] cannot be a field in field %s %s", __func__, typeName, typeName, fieldName);
            break;
        case CXType_Pointer:
        {
            CXType innerType = clang_getPointeeType(clangType);
            // Only class type can be a pointer so check that
            CXCursor classDecl = clang_getTypeDeclaration(innerType);
            bIsValid = clang_getCursorKind(classDecl) == CXCursor_ClassDecl && isReflectedClass(classDecl);
            if (!bIsValid)
            {
                LOG_ERROR("ParserHelper", "%s() : Pointer type[%s] must be a class that is reflected in field %s %s", __func__, typeName, typeName, fieldName);
            }
            break;
        }
        case CXType_Vector:
        {
            CXType innerType = clang_getElementType(clangType);
            bIsValid = isValidFieldType(innerType, clang_getNullCursor());
            if (!bIsValid)
            {
                LOG_ERROR("ParserHelper", "%s() : Vector type[%s] must hold valid type that is reflected in field %s %s", __func__, typeName, typeName, fieldName);
            }
            break;
        }
        case CXType_Record:
        {
            // If not reference/pointer or vector, Then it must be POD or struct/class that is reflected or one of specialized struct or valid map/set/vector/pair
            if(!ParserHelper::isBuiltinType(clangType))
            {
                CXCursor typeDecl = clang_getTypeDeclaration(clangType);
                // Either reflected struct or specialized struct or class templates like std::vector or class decl like Matrix3 or Vector3D
                bIsValid = (clang_getCursorKind(typeDecl) == CXCursor_StructDecl 
                        || clang_getCursorKind(typeDecl) == CXCursor_ClassDecl
                        || clang_getCursorKind(typeDecl) == CXCursor_ClassTemplate)
                    && (isReflectedClass(typeDecl) 
                        || isSpecializedType(clangType, fieldCursor)
                        || isCustomType(clangType, fieldCursor));
                if (!bIsValid)
                {
                    LOG_ERROR("ParserHelper", "%s() : Type %s is not valid field type", __func__, typeName);
                }
            }
            break;
        }
        case CXType_Elaborated:
        {
            bIsValid = isValidFieldType(clang_getCanonicalType(clangType), fieldCursor);
            break;
        }
        case CXType_ConstantArray:
        case CXType_IncompleteArray:
        case CXType_DependentSizedArray:
        case CXType_VariableArray:
        default:
            break;
        }
    }
    return bIsValid;
}