/*!
 * \file DataFormulator.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Generator/SourceGenerator.h"
#include "ModuleSources.h"
#include "Logger/Logger.h"
#include "Parser/ParserHelper.h"
#include "GeneratorConsts.h"
#include "Parser/ClangWrappers.h"
#include "Property/PropertyHelper.h"

struct LocalContext
{
    SourceGeneratorContext* srcGenContext = nullptr;
    MustacheContext* parentContext = nullptr;
    MustacheContext* parentRegisterContext = nullptr;
    // Cursors that are in same child level but are not handled needs to be handled separately
    std::vector<CXCursor> unhandledSibilings;

    void* pNext = nullptr;
};

struct ClassParseContext
{
    bool bHasConstructor = false;
};

void visitTUCusor(CXCursor cursor, SourceGeneratorContext* srcGenContext);

template <typename FmtType, typename... Args>
void parseFailed(CXCursor cursor, SourceGeneratorContext* srcGenContext, const TChar* funcName, FmtType&& fmtMsg, Args&&... args)
{
    LOG("SourceGenerator", "%s ERROR %s() : Reflection parsing failed - %s", clang_getCursorLocation(cursor), funcName
        , StringFormat::format(std::forward<FmtType>(fmtMsg), std::forward<Args>(args)...));
    srcGenContext->bGenerated = false;
}

template <StringLiteral MetaDataTag, StringLiteral MetaFlagsTag>
FORCE_INLINE void setTypeMetaInfo(MustacheContext& typeContext, const std::vector<String>& metaData, const std::vector<String>& metaFlags)
{
    std::vector<String> metaDataInitList(metaData.size());
    for (uint32 i = 0; i < metaData.size(); ++i)
    {
        metaDataInitList[i] = TCHAR("new " + metaData[i]);
    }

    typeContext.args[MetaDataTag.value] = String::join(metaDataInitList.cbegin(), metaDataInitList.cend(), TCHAR(", "));
    if (metaFlags.empty())
    {
        typeContext.args[MetaFlagsTag.value] = TCHAR("0");
    }
    else
    {
        typeContext.args[MetaFlagsTag.value] = String::join(metaFlags.cbegin(), metaFlags.cend(), TCHAR(" | "));
    }
}

FORCE_INLINE void addQualifiedType(const String& typeName, const String& sanitizedTypeName, SourceGeneratorContext* srcGenContext)
{
    MustacheContext& qualifiedContext = srcGenContext->qualifiedTypes.emplace_back();
    MustacheContext& allRegisterdTypeCntxt = srcGenContext->allRegisteredypes.emplace_back();

    // Setup contexts
    allRegisterdTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = typeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::NOINIT_BRANCH_TAG] = false;
    allRegisterdTypeCntxt.args[GeneratorConsts::PROPERTYTYPENAME_TAG] = GeneratorConsts::BASEPROPERTY;
    allRegisterdTypeCntxt.args[GeneratorConsts::REGISTERFUNCNAME_TAG] = GeneratorConsts::REGISTERTYPEFACTORY_FUNC;

    qualifiedContext.args[GeneratorConsts::TYPENAME_TAG] = typeName;
    qualifiedContext.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;

    srcGenContext->addedSymbols.insert(sanitizedTypeName);
}

void generatePrereqTypes(CXType type, SourceGeneratorContext* srcGenContext);

void visitEnums(CXCursor cursor, SourceGeneratorContext* srcGenContext)
{
    if (!ParserHelper::isReflectedDecl(cursor))
    {
        return;
    }

    MustacheContext& allRegisterdTypeCntxt = srcGenContext->allRegisteredypes.emplace_back();
    MustacheContext& enumCntxt = srcGenContext->enumTypes.emplace_back();

    const String enumMetaStr = ParserHelper::getCursorMetaString(cursor);
    std::vector<String> metaFlags, metaData, buildFlags;
    ParserHelper::parseEnumMeta(metaFlags, metaData, buildFlags, enumMetaStr);

    // Why getting from canonical type? Because it gives name with all the scopes prefixed. We do not have to handle parent namespace or types
    const String enumTypeName = CXStringWrapper(clang_getTypeSpelling(clang_getCanonicalType(clang_getCursorType(cursor)))).toString();
    const String sanitizedTypeName = PropertyHelper::getValidSymbolName(enumTypeName);

    // Setup source contexts
    allRegisterdTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = enumTypeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::NOINIT_BRANCH_TAG] = false;
    allRegisterdTypeCntxt.args[GeneratorConsts::PROPERTYTYPENAME_TAG] = GeneratorConsts::ENUMPROPERTY;
    allRegisterdTypeCntxt.args[GeneratorConsts::REGISTERFUNCNAME_TAG] = GeneratorConsts::REGISTERENUMFACTORY_FUNC;

    setTypeMetaInfo<GeneratorConsts::TYPEMETADATA_TAG.Literal, GeneratorConsts::TYPEMETAFLAGS_TAG.Literal>(enumCntxt, metaData, metaFlags);
    enumCntxt.args[GeneratorConsts::TYPENAME_TAG] = enumTypeName;
    enumCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;

    // Now fill members
    struct EnumCanBeUsedAsFlagData
    {
        // to hold persistent flag info between enum constants
        uint64 flags = 0;
        bool bCanBeUsedAsFlags = true;
    } enumFieldsCanBeFlags;
    LocalContext localCtx
    {
        .srcGenContext = srcGenContext,
        .parentContext = &enumCntxt,
        .parentRegisterContext = &allRegisterdTypeCntxt,
        .pNext = &enumFieldsCanBeFlags
    };
    clang_visitChildren(cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            LocalContext* localCntxt = ((LocalContext*)(clientData));
            CXCursorKind cursorKind = clang_getCursorKind(c);
            String cursorName(CXStringWrapper(clang_getCursorSpelling(c)).toString());
            switch (cursorKind)
            {
            case CXCursor_EnumConstantDecl:
            {
                uint64 enumVal = clang_getEnumConstantDeclUnsignedValue(c);
                String enumConstMetaStr = ParserHelper::getCursorMetaString(c);
                std::vector<String> enumConstMetaFlags, enumConstMetaData, buildFlags;
                ParserHelper::parseEnumMeta(enumConstMetaFlags, enumConstMetaData, buildFlags, enumConstMetaStr);
                // Write enum constant context
                MustacheContext& enumConstContext = localCntxt->parentContext->sectionContexts[GeneratorConsts::ENUMFIELDS_SECTION_TAG].emplace_back();
                enumConstContext.args[GeneratorConsts::ENUMFIELDNAME_TAG] = cursorName;
                enumConstContext.args[GeneratorConsts::ENUMFIELDVALUE_TAG] = enumVal;
                setTypeMetaInfo<GeneratorConsts::ENUMFIELDMETADATA_TAG.Literal, GeneratorConsts::ENUMFIELDMETAFLAGS_TAG.Literal>
                    (enumConstContext, enumConstMetaData, enumConstMetaFlags);

                // Check and set if can be used as flags, Only if each enum const value has one flag set and it does not overlap with any other flags it can be used as flags
                EnumCanBeUsedAsFlagData& persistentData = *(EnumCanBeUsedAsFlagData*)(localCntxt->pNext);
                persistentData.bCanBeUsedAsFlags = persistentData.bCanBeUsedAsFlags && ONE_BIT_SET(enumVal) && BIT_NOT_SET(persistentData.flags, enumVal);
                persistentData.flags |= enumVal;
                break;
            }
            default:
                localCntxt->unhandledSibilings.emplace_back(c);
                break;
            }
            return CXChildVisit_Continue;
        }
    , &localCtx);

    enumCntxt.args[GeneratorConsts::CANUSEASFLAGS_TAG] = enumFieldsCanBeFlags.bCanBeUsedAsFlags;

    for (CXCursor c : localCtx.unhandledSibilings)
    {
        visitTUCusor(c, srcGenContext);
    }
}

void visitMemberField(CXCursor cursor, LocalContext& localCntxt)
{
    if (!ParserHelper::isReflectedDecl(cursor))
    {
        return;
    }

    String fieldName(CXStringWrapper(clang_getCursorSpelling(cursor)).toString());

    CXType fieldType = clang_getCursorType(cursor);
    String typeName = ParserHelper::getCursorTypeName(cursor);

    if (!ParserHelper::isValidFieldType(fieldType, cursor))
    {
        parseFailed(cursor, localCntxt.srcGenContext, __func__
            , TCHAR("Invalid member field %s"), fieldName);
        return;
    }

    const String fieldMetaStr = ParserHelper::getCursorMetaString(cursor);
    std::vector<String> metaFlags, metaData, buildFlags;
    ParserHelper::parseFieldMeta(metaFlags, metaData, buildFlags, fieldMetaStr);

    // Generate prerequisite types
    generatePrereqTypes(fieldType, localCntxt.srcGenContext);

    // Setup context
    MustacheContext& context = (clang_getCursorKind(cursor) == CXCursor_FieldDecl)
        ? localCntxt.parentContext->sectionContexts[GeneratorConsts::MEMBERFIELDS_SECTION_TAG].emplace_back()
        : localCntxt.parentContext->sectionContexts[GeneratorConsts::STATICFIELDS_SECTION_TAG].emplace_back();
    setTypeMetaInfo<GeneratorConsts::FIELDMETADATA_TAG.Literal, GeneratorConsts::FIELDMETAFLAGS_TAG.Literal>(context, metaData, metaFlags);
    context.args[GeneratorConsts::FIELDNAME_TAG] = fieldName;
    context.args[GeneratorConsts::FIELDTYPENAME_TAG] = typeName;
    context.args[GeneratorConsts::ACCESSSPECIFIER_TAG] = ParserHelper::accessSpecifierName(cursor);
}

void visitMemberCppMethods(CXCursor cursor, LocalContext& localCntxt)
{
    // We reflect all constructors even not marked for meta reflection
    if (!(ParserHelper::isReflectedDecl(cursor) || clang_getCursorKind(cursor) == CXCursor_Constructor))
    {
        return;
    }

    String funcName(CXStringWrapper(clang_getCursorSpelling(cursor)).toString());

    if (!ParserHelper::isValidFunction(cursor))
    {
        parseFailed(cursor, localCntxt.srcGenContext, __func__
            , TCHAR("Invalid function %s"), funcName);
        return;
    }

    // This is the condition to allow setting up class/struct data before invoking constructor
    // Default constructors always zero the data before invoking
    if (clang_CXXMethod_isDefaulted(cursor))
    {
        parseFailed(cursor, localCntxt.srcGenContext, __func__
            , TCHAR("Default functions/Constructors are not allowed for reflected types %s"), funcName);
        return;
    }

    int32 bIsStatic = clang_CXXMethod_isStatic(cursor);
    int32 bIsConst = clang_CXXMethod_isConst(cursor);
    const String funcMetaStr = ParserHelper::getCursorMetaString(cursor);
    std::vector<String> metaFlags, metaData, buildFlags;
    ParserHelper::parseFunctionMeta(metaFlags, metaData, buildFlags, funcMetaStr);

    CXType funcRetType = clang_getCursorResultType(cursor);
    int32 paramsCount = clang_Cursor_getNumArguments(cursor);
    std::vector<String> paramsList(paramsCount);
    std::vector<String> paramsName(paramsCount);
    for (uint32 i = 0; i < paramsCount; ++i)
    {
        CXCursor paramCursor = clang_Cursor_getArgument(cursor, i);
        // Generate prerequisites for param cursors
        generatePrereqTypes(clang_getCursorType(paramCursor), localCntxt.srcGenContext);

        String paramTypeName = ParserHelper::getCursorTypeName(paramCursor);
        String paramName = CXStringWrapper(clang_getCursorSpelling(paramCursor)).toString();
        paramsList[i] = paramTypeName;
        paramsName[i] = paramName;
    }

    MustacheContext* contextPtr = nullptr;
    if (clang_getCursorKind(cursor) == CXCursor_Constructor)
    {
        MustacheContext& context = localCntxt.parentContext->sectionContexts[GeneratorConsts::CONSTRUCTORS_SECTION_TAG].emplace_back();
        setTypeMetaInfo<GeneratorConsts::CONSTRUCTORMETADATA_TAG.Literal, GeneratorConsts::CONSTRUCTORMETAFLAGS_TAG.Literal>(context, metaData, metaFlags);
        // For now only class has any valid next pointer, If struct also needs it then we must handle it differently
        if (localCntxt.pNext != nullptr)
        {
            ClassParseContext* classCntx = (ClassParseContext*)localCntxt.pNext;
            classCntx->bHasConstructor = true;
        }
        contextPtr = &context;
    }
    else
    {
        generatePrereqTypes(funcRetType, localCntxt.srcGenContext);
        String returnTypeName = CXStringWrapper(clang_getTypeSpelling(funcRetType)).toString();
        if (!ParserHelper::isBuiltinType(ParserHelper::getTypeReferred(funcRetType, clang_getNullCursor())))
        {
            // Why getting from canonical type? Because it gives name with all the scopes prefixed. We do not have to handle parent namespace or types
            returnTypeName = CXStringWrapper(clang_getTypeSpelling(clang_getCanonicalType(funcRetType))).toString();
        }

        MustacheContext& context = (bIsStatic)
            ? localCntxt.parentContext->sectionContexts[GeneratorConsts::STATICFUNCS_SECTION_TAG].emplace_back()
            : localCntxt.parentContext->sectionContexts[GeneratorConsts::MEMBERFUNCS_SECTION_TAG].emplace_back();
        setTypeMetaInfo<GeneratorConsts::FUNCMETADATA_TAG.Literal, GeneratorConsts::FUNCMETAFLAGS_TAG.Literal>(context, metaData, metaFlags);
        context.args[GeneratorConsts::FUNCTIONNAME_TAG] = funcName;
        context.args[GeneratorConsts::FUNCCONST_BRANCH_TAG] = bIsConst;
        context.args[GeneratorConsts::RETURNTYPENAME_TAG] = returnTypeName;

        contextPtr = &context;
    }
    contextPtr->args[GeneratorConsts::ACCESSSPECIFIER_TAG] = ParserHelper::accessSpecifierName(cursor);
    contextPtr->args[GeneratorConsts::PARAMLIST_TAG] = String::join(paramsList.cbegin(), paramsList.cend(), TCHAR(", "));

    std::vector<MustacheContext>& paramsListContexts = contextPtr->sectionContexts[GeneratorConsts::PARAMSLISTCONTEXT_SECTION_TAG];
    paramsListContexts.resize(paramsList.size());
    for (uint32 i = 0; i < paramsList.size(); ++i)
    {
        paramsListContexts[i].args[GeneratorConsts::PARAMNAME_TAG] = paramsName[i];
        paramsListContexts[i].args[GeneratorConsts::PARAMTYPENAME_TAG] = paramsList[i];
    }
}

void visitClassMember(CXCursor cursor, LocalContext& localCntxt)
{
    CXCursorKind cursorKind = clang_getCursorKind(cursor);
    CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(cursor)));

    switch (cursorKind)
    {
    case CXCursor_CXXBaseSpecifier:
    {
        CXCursor baseClass = clang_getTypeDeclaration(clang_getCursorType(cursor));
        if (clang_Cursor_isNull(baseClass))
        {
            parseFailed(cursor, localCntxt.srcGenContext, __func__
                , TCHAR("Cannot find declaration of base class %s"), cursorName);
        }
        else if (ParserHelper::isReflectedClass(baseClass))
        {
            MustacheContext& baseClassCntxt = localCntxt.parentContext->sectionContexts[GeneratorConsts::BASECLASSES_SECTION_TAG].emplace_back();
            CXStringWrapper baseClassName = CXStringWrapper(clang_getTypeSpelling(clang_getCursorType(baseClass)));
            baseClassCntxt.args[GeneratorConsts::BASECLASSTYPENAME_TAG] = baseClassName;
        }
        break;
    }
    case CXCursor_VarDecl:
    case CXCursor_FieldDecl:
        visitMemberField(cursor, localCntxt);
        break;
    case CXCursor_Constructor:
    case CXCursor_CXXMethod:
    case CXCursor_FunctionTemplate:
        // All member functions including static member functions
        visitMemberCppMethods(cursor, localCntxt);
        break;
    default:
        localCntxt.unhandledSibilings.emplace_back(cursor);
        break;
    }
}

void visitStructs(CXCursor cursor, SourceGeneratorContext* srcGenContext)
{
    if (!ParserHelper::isReflectedClass(cursor))
    {
        return;
    }
    MustacheContext& headerReflectTypeCntxt = srcGenContext->headerReflectTypes.emplace_back();

    MustacheContext& allRegisterdTypeCntxt = srcGenContext->allRegisteredypes.emplace_back();
    MustacheContext& structCntxt = srcGenContext->classTypes.emplace_back();

    const String classMetaStr = ParserHelper::getCursorMetaString(cursor);
    std::vector<String> metaFlags, metaData, buildFlags;
    ParserHelper::parseClassMeta(metaFlags, metaData, buildFlags, classMetaStr);

    // Why getting from canonical type? Because it gives name with all the scopes prefixed. We do not have to handle parent namespace or types
    const String structCanonicalTypeName = CXStringWrapper(clang_getTypeSpelling(clang_getCanonicalType(clang_getCursorType(cursor)))).toString();
    const String structTypeName = CXStringWrapper(clang_getCursorSpelling(cursor)).toString();
    const String sanitizedTypeName = PropertyHelper::getValidSymbolName(structCanonicalTypeName);
    CXSourceLocation generatedCodesSrcLoc = clang_getCursorLocation(ParserHelper::getGeneratedCodeCursor(cursor));
    uint32 genCodesLineNum = 0;
    clang_getFileLocation(generatedCodesSrcLoc, nullptr, &genCodesLineNum, nullptr, nullptr);
    bool bIsAbstract = !!clang_CXXRecord_isAbstract(cursor);
    const bool bNoExport = std::find(buildFlags.cbegin(), buildFlags.cend(), GeneratorConsts::NOEXPORT_FLAG.toString()) != buildFlags.cend();
    const bool bHasOverridenCtorPolicy = ParserHelper::hasOverridenCtorPolicy(cursor);

    // Setup header context
    headerReflectTypeCntxt.args[GeneratorConsts::ISCLASS_BRANCH_TAG] = false;
    headerReflectTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = structCanonicalTypeName;
    headerReflectTypeCntxt.args[GeneratorConsts::SIMPLE_TYPENAME_TAG] = structTypeName;
    headerReflectTypeCntxt.args[GeneratorConsts::LINENUMBER_TAG] = genCodesLineNum;
    headerReflectTypeCntxt.args[GeneratorConsts::ISBASETYPE_BRANCH_TAG] = true;// We do not support inheritance in struct
    headerReflectTypeCntxt.args[GeneratorConsts::DEFINECTORPOLICY_BRANCH_TAG] = !bHasOverridenCtorPolicy;
    // If class is explicitly mark NoExport then do not export
    headerReflectTypeCntxt.args[GeneratorConsts::NOEXPORT_BRANCH_TAG] = bNoExport;

    // Setup source contexts
    allRegisterdTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = structCanonicalTypeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::NOINIT_BRANCH_TAG] = false;
    allRegisterdTypeCntxt.args[GeneratorConsts::PROPERTYTYPENAME_TAG] = GeneratorConsts::CLASSPROPERTY;
    allRegisterdTypeCntxt.args[GeneratorConsts::REGISTERFUNCNAME_TAG] = GeneratorConsts::REGISTERSTRUCTFACTORY_FUNC;

    setTypeMetaInfo<GeneratorConsts::TYPEMETADATA_TAG.Literal, GeneratorConsts::TYPEMETAFLAGS_TAG.Literal>(structCntxt, metaData, metaFlags);
    structCntxt.args[GeneratorConsts::ISABSTRACT_TAG] = bIsAbstract;
    structCntxt.args[GeneratorConsts::TYPENAME_TAG] = structCanonicalTypeName;
    structCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;


    // Now fill members
    
    // Class and Struct have constructor and they return there own pointers so we generate class/struct pointer even when not used anywhere yet
    const String structPtrTypeName = structCanonicalTypeName + TCHAR(" *");
    const String structPtrSanitizedName = PropertyHelper::getValidSymbolName(structPtrTypeName);
    if (!srcGenContext->addedSymbols.contains(structPtrSanitizedName))
    {
        addQualifiedType(structPtrTypeName, structPtrSanitizedName, srcGenContext);
    }

    LocalContext localCtx
    {
        .srcGenContext = srcGenContext,
        .parentContext = &structCntxt,
        .parentRegisterContext = &allRegisterdTypeCntxt
    };

    clang_visitChildren(cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            // It is okay to use same visitor function as class here. Only BaseProperty gets populated and It has no meaning now
            visitClassMember(c, *(LocalContext*)(clientData));
            return CXChildVisit_Continue;
        }
    , &localCtx);

    for (CXCursor c : localCtx.unhandledSibilings)
    {
        visitTUCusor(c, srcGenContext);
    }
}

void visitClasses(CXCursor cursor, SourceGeneratorContext* srcGenContext)
{
    if (!ParserHelper::isReflectedClass(cursor))
    {
        return;
    }
    MustacheContext& headerReflectTypeCntxt = srcGenContext->headerReflectTypes.emplace_back();

    MustacheContext& allRegisterdTypeCntxt = srcGenContext->allRegisteredypes.emplace_back();
    MustacheContext& classCntxt = srcGenContext->classTypes.emplace_back();

    const String classMetaStr = ParserHelper::getCursorMetaString(cursor);
    std::vector<String> metaFlags, metaData, buildFlags;
    ParserHelper::parseClassMeta(metaFlags, metaData, buildFlags, classMetaStr);

    const String classCanonicalTypeName = CXStringWrapper(clang_getTypeSpelling(clang_getCanonicalType(clang_getCursorType(cursor)))).toString();
    const String classTypeName = CXStringWrapper(clang_getCursorSpelling(cursor)).toString();
    const String sanitizedTypeName = PropertyHelper::getValidSymbolName(classCanonicalTypeName);
    CXSourceLocation generatedCodesSrcLoc = clang_getCursorLocation(ParserHelper::getGeneratedCodeCursor(cursor));
    uint32 genCodesLineNum = 0;
    clang_getFileLocation(generatedCodesSrcLoc, nullptr, &genCodesLineNum, nullptr, nullptr);
    const bool bIsAbstract = !!clang_CXXRecord_isAbstract(cursor);
    const bool bIsBaseType = std::find(buildFlags.cbegin(), buildFlags.cend(), GeneratorConsts::BASETYPE_FLAG.toString()) != buildFlags.cend();
    const bool bNoExport = std::find(buildFlags.cbegin(), buildFlags.cend(), GeneratorConsts::NOEXPORT_FLAG.toString()) != buildFlags.cend();
    const bool bHasOverridenCtorPolicy = ParserHelper::hasOverridenCtorPolicy(cursor);

    // Setup header context
    headerReflectTypeCntxt.args[GeneratorConsts::ISCLASS_BRANCH_TAG] = true;
    headerReflectTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = classCanonicalTypeName;
    headerReflectTypeCntxt.args[GeneratorConsts::SIMPLE_TYPENAME_TAG] = classTypeName;
    headerReflectTypeCntxt.args[GeneratorConsts::LINENUMBER_TAG] = genCodesLineNum;
    headerReflectTypeCntxt.args[GeneratorConsts::ISBASETYPE_BRANCH_TAG] = bIsBaseType;
    headerReflectTypeCntxt.args[GeneratorConsts::DEFINECTORPOLICY_BRANCH_TAG] = !bHasOverridenCtorPolicy;
    // If class is explicitly mark NoExport then do not export
    headerReflectTypeCntxt.args[GeneratorConsts::NOEXPORT_BRANCH_TAG] = bNoExport;

    // Setup source contexts
    allRegisterdTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = classCanonicalTypeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;
    allRegisterdTypeCntxt.args[GeneratorConsts::NOINIT_BRANCH_TAG] = false;
    allRegisterdTypeCntxt.args[GeneratorConsts::PROPERTYTYPENAME_TAG] = GeneratorConsts::CLASSPROPERTY;
    allRegisterdTypeCntxt.args[GeneratorConsts::REGISTERFUNCNAME_TAG] = GeneratorConsts::REGISTERCLASSFACTORY_FUNC;

    setTypeMetaInfo<GeneratorConsts::TYPEMETADATA_TAG.Literal, GeneratorConsts::TYPEMETAFLAGS_TAG.Literal>(classCntxt, metaData, metaFlags);
    classCntxt.args[GeneratorConsts::ISABSTRACT_TAG] = bIsAbstract;
    classCntxt.args[GeneratorConsts::TYPENAME_TAG] = classCanonicalTypeName;
    classCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;


    // Now fill members

    // Class and Struct have constructor and they return there own pointers so we generate class/struct pointer even when not used anywhere yet
    const String classPtrTypeName = classCanonicalTypeName + TCHAR(" *");
    const String classPtrSanitizedName = PropertyHelper::getValidSymbolName(classPtrTypeName);
    if (!srcGenContext->addedSymbols.contains(classPtrSanitizedName))
    {
        addQualifiedType(classPtrTypeName, classPtrSanitizedName, srcGenContext);
    }

    ClassParseContext classParseCntx;
    LocalContext classLocalCtx
    {
        .srcGenContext = srcGenContext,
        .parentContext = &classCntxt,
        .parentRegisterContext = &allRegisterdTypeCntxt,
        .pNext = &classParseCntx
    };

    // Visit all members
    clang_visitChildren(cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            visitClassMember(c, *(LocalContext*)(clientData));
            return CXChildVisit_Continue;
        }
    , &classLocalCtx);

    headerReflectTypeCntxt.args[GeneratorConsts::IFGENERATECTOR_BRANCH_TAG] = !classParseCntx.bHasConstructor;

    for (CXCursor c : classLocalCtx.unhandledSibilings)
    {
        visitTUCusor(c, srcGenContext);
    }
}

void generatePrereqTypes(CXType type, SourceGeneratorContext* srcGenContext)
{
    if (type.kind == CXType_Invalid)
    {
        return;
    }

    CXCursor nullCursor = clang_getNullCursor();
    CXType canonicalType = clang_getCanonicalType(type);
    CXType referredType = ParserHelper::getTypeReferred(canonicalType, nullCursor);
    // POD and special types are already generated
    if (ParserHelper::isBuiltinType(referredType) || ParserHelper::isSpecializedType(referredType, nullCursor))
    {
        return;
    }
    String typeName = CXStringWrapper(clang_getTypeSpelling(canonicalType)).toString();
    String sanitizedTypeName = PropertyHelper::getValidSymbolName(typeName);
    // If type have any reference or pointer, Or if it is const qualified then we need to create qualified property
    bool bIsQualified = (clang_isConstQualifiedType(referredType) 
        || type.kind == CXType_LValueReference 
        || type.kind == CXType_RValueReference 
        || type.kind == CXType_Pointer);

    // If this symbol is already added in current TU then skip it
    if (srcGenContext->addedSymbols.contains(sanitizedTypeName))
    {
        return;
    }

    // Custom types needs customized generation
    if (ParserHelper::isCustomType(referredType, nullCursor))
    {
        std::vector<MustacheContext>* customTypeContexts = nullptr;

        // Add more if needed
        CXType type1, type2;
        if (ParserHelper::getContainerElementType(type1, referredType, nullCursor))
        {
            generatePrereqTypes(type1, srcGenContext);
            customTypeContexts = &srcGenContext->containerTypes;
        }
        else if(ParserHelper::getPairElementTypes(type1, type2, referredType, nullCursor))
        {
            generatePrereqTypes(type1, srcGenContext);
            generatePrereqTypes(type2, srcGenContext);
            customTypeContexts = &srcGenContext->pairTypes;
        }
        else if (ParserHelper::getMapElementTypes(type1, type2, referredType, nullCursor))
        {
            generatePrereqTypes(type1, srcGenContext);
            generatePrereqTypes(type2, srcGenContext);
            customTypeContexts = &srcGenContext->mapTypes;
        }
        else
        {
            LOG_ERROR("SourceGenerator", "%s() : Type %s is not fully supported custom type", __func__, clang_getTypeSpelling(referredType));
            srcGenContext->bGenerated = false;
            return;
        }
        srcGenContext->addedSymbols.insert(sanitizedTypeName);

        MustacheContext* typeContext = nullptr;
        // Why do like below? switch context base on qualified state? - Since we did the symbol added check for currently generating type
        // Which might be qualified so inside qualified condition we do base symbol check and add it as needed

        // If qualified then we need to create non const qualified type as well, In which case below if scope will be creating base custom property
        // If the type itself is unqualified then the above typeContext will be one creating base custom property
        if (bIsQualified)
        {
            // Type name without any qualifiers
            String baseTypeName = ParserHelper::getNonConstTypeName(canonicalType, nullCursor);
            String baseSanitizedTypeName = PropertyHelper::getValidSymbolName(baseTypeName);
            
            // If base type is not already added then add it here
            if (!srcGenContext->addedSymbols.contains(baseSanitizedTypeName))
            {
                MustacheContext& customTypeContext = customTypeContexts->emplace_back();
                MustacheContext& allRegisterdTypeCntxt = srcGenContext->allRegisteredypes.emplace_back();

                // Setup contexts
                allRegisterdTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = baseTypeName;
                allRegisterdTypeCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = baseSanitizedTypeName;
                allRegisterdTypeCntxt.args[GeneratorConsts::NOINIT_BRANCH_TAG] = false;
                allRegisterdTypeCntxt.args[GeneratorConsts::PROPERTYTYPENAME_TAG] = GeneratorConsts::BASEPROPERTY;
                allRegisterdTypeCntxt.args[GeneratorConsts::REGISTERFUNCNAME_TAG] = GeneratorConsts::REGISTERTYPEFACTORY_FUNC;

                customTypeContext.args[GeneratorConsts::TYPENAME_TAG] = baseTypeName;
                customTypeContext.args[GeneratorConsts::SANITIZEDNAME_TAG] = baseSanitizedTypeName;

                srcGenContext->addedSymbols.insert(baseSanitizedTypeName);
            }

            // Switch the tempContext to create inside qualified types
            typeContext = &srcGenContext->qualifiedTypes.emplace_back();
        }
        else
        {
            typeContext = &customTypeContexts->emplace_back();
        }

        MustacheContext& allRegisterdTypeCntxt = srcGenContext->allRegisteredypes.emplace_back();

        // Setup contexts
        allRegisterdTypeCntxt.args[GeneratorConsts::TYPENAME_TAG] = typeName;
        allRegisterdTypeCntxt.args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;
        allRegisterdTypeCntxt.args[GeneratorConsts::NOINIT_BRANCH_TAG] = false;
        allRegisterdTypeCntxt.args[GeneratorConsts::PROPERTYTYPENAME_TAG] = GeneratorConsts::BASEPROPERTY;
        allRegisterdTypeCntxt.args[GeneratorConsts::REGISTERFUNCNAME_TAG] = GeneratorConsts::REGISTERTYPEFACTORY_FUNC;

        typeContext->args[GeneratorConsts::TYPENAME_TAG] = typeName;
        typeContext->args[GeneratorConsts::SANITIZEDNAME_TAG] = sanitizedTypeName;
    }
    else // Anything other than custom types needs declaration available to be acceptable reflected type
    {
        CXCursor typeDecl = clang_getTypeDeclaration(referredType);
        if (clang_Cursor_isNull(typeDecl))
        {
            LOG_ERROR("SourceGenerator", "%s() : Type %s do not have any declaration and cannot be reflected", __func__, clang_getTypeSpelling(referredType));
            srcGenContext->bGenerated = false;
            return;
        }

        if ((clang_getCursorKind(typeDecl) == CXCursor_EnumDecl && ParserHelper::isReflectedDecl(typeDecl))
            || ((clang_getCursorKind(typeDecl) == CXCursor_ClassDecl || clang_getCursorKind(typeDecl) == CXCursor_StructDecl)
                && ParserHelper::isReflectedClass(typeDecl)))
        {
            if (bIsQualified)
            {
                addQualifiedType(typeName, sanitizedTypeName, srcGenContext);
                // We do not need to generate inner prerequisite types here
            }
        }
        else
        {
            parseFailed(typeDecl, srcGenContext, __func__
                , TCHAR("Type %s declaration is not reflected"), clang_getTypeSpelling(referredType));
            return;
        }
    }
}

void visitTUCusor(CXCursor cursor, SourceGeneratorContext* srcGenContext)
{
    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    switch (cursorKind)
    {
    case CXCursor_StructDecl:
        visitStructs(cursor, srcGenContext);
        return;
    case CXCursor_ClassDecl:
        visitClasses(cursor, srcGenContext);
        return;
    case CXCursor_EnumDecl:
        visitEnums(cursor, srcGenContext);
        return;
    case CXCursor_Namespace:
    {
        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                visitTUCusor(c, (SourceGeneratorContext*)(clientData));
                return CXChildVisit_Continue;
            }
        , srcGenContext);
        return;
    }
    }
}

FORCE_INLINE void parseSource(const SourceInformation* srcInfo, SourceGeneratorContext& srcGenContext)
{
    CXCursor cursor = clang_getTranslationUnitCursor(srcInfo->tu);
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data)
        {
            // If this symbol is from this Source file?
            // CXSourceLocation is not need to be freed
            if (!!clang_Location_isFromMainFile(clang_getCursorLocation(c)))
            {
                visitTUCusor(c, (SourceGeneratorContext*)(client_data));
            }
            // Continue to next Cursor in TU
            return CXChildVisit_Continue;
        },
        &srcGenContext);
}

void SourceGenerator::parseSources()
{
    for (std::pair<const SourceInformation* const, SourceGeneratorContext>& source : sourceToGenCntxt)
    {
        parseSource(source.first, source.second);
    }
}