/*!
 * \file SampleCode.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "SampleCode.h"
#include "IReflectionRuntime.h"
#include "Modules/ModuleManager.h"
#include "Parser/ClangWrappers.h"
#include "Parser/ParserHelper.h"
#include "Property/ContainerProperty.h"
#include "Property/Property.h"
#include "Property/PropertyHelper.h"
#include "String/MustacheFormatString.h"
#include "String/StringRegex.h"
#include "Types/Containers/ArrayView.h"
#include "Types/FunctionTypes.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Types/PropertyTypes.h"
#include "Types/TypesInfo.h"

#include <iostream>
#include <regex>
#include <set>

namespace CppReflectionParser
{
void printDiagnostics(CXDiagnostic diagnostic, uint32 formatOptions)
{
    CXDiagnosticSet childDiags = clang_getChildDiagnostics(diagnostic);
    const uint32 childDiagsNum = clang_getNumDiagnosticsInSet(childDiags);

    CXStringRef diagnosticStr(new CXStringWrapper(clang_formatDiagnostic(diagnostic, formatOptions)));
    LOG_WARN("Diagnostics", "%s", diagnosticStr);
    for (int32 i = 0; i < childDiagsNum; ++i)
    {
        auto childDiagnostic = clang_getDiagnosticInSet(childDiags, i);
        printDiagnostics(childDiagnostic, formatOptions);
        clang_disposeDiagnostic(childDiagnostic);
    }
}

String accessSpecifierStr(CXCursor cursor)
{
    String access;
    CX_CXXAccessSpecifier currentScopeAccess = clang_getCXXAccessSpecifier(cursor);
    switch (currentScopeAccess)
    {
    case CX_CXXPublic:
        access = TCHAR("public");
        break;
    case CX_CXXProtected:
        access = TCHAR("protected");
        break;
    case CX_CXXPrivate:
        access = TCHAR("private");
        break;
    case CX_CXXInvalidAccessSpecifier:
    default:
        access = TCHAR("Invalid");
        break;
    }
    return access;
}

void printJustTypeInfo(CXType type)
{
    CXType canonicalType = clang_getCanonicalType(type);
    CXStringRef typeName(new CXStringWrapper(clang_getTypeSpelling(type)));
    CXType innerType = clang_getPointeeType(canonicalType);
    switch (canonicalType.kind)
    {
    case CXType_RValueReference:
        LOG("CppReflectionParser", "Type %s is a r-value, Referred type %s(Is POD %d)", typeName, clang_getTypeSpelling(innerType),
            clang_isPODType(innerType));
        break;
    case CXType_LValueReference:
        LOG("CppReflectionParser", "Type %s is a l-value, Referred type %s(Is POD %d)", typeName, clang_getTypeSpelling(innerType),
            clang_isPODType(innerType));
        break;
    case CXType_Pointer:
    {
        int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
        LOG("CppReflectionParser", "Type %s - Inner type is %s and is const? %s(Is POD %d)", typeName, clang_getTypeSpelling(innerType),
            !!(bIsInnerTypeConst) ? "true" : "false", clang_isPODType(innerType));
        break;
    }
    case CXType_ConstantArray:
        LOG("CppReflectionParser", "Type %s container element count %d", typeName, clang_getNumElements(type));
    case CXType_IncompleteArray:
    case CXType_DependentSizedArray:
    case CXType_Vector:
    case CXType_VariableArray:
    {
        int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
        LOG("CppReflectionParser", "Type %s - Inner type is %s and is const? %s(Is POD %d)", typeName, clang_getTypeSpelling(innerType),
            !!(bIsInnerTypeConst) ? "true" : "false", clang_isPODType(innerType));
        break;
    }
    default:
        break;
    }
}

void printVariableTypeInfo(CXCursor cursor, SourceParsedInfo &srcParsedInfo, CXType fieldType, CXType fieldCanonicalType)
{
    CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(cursor)));

    // The type can be considered const if its container is const or the type itself is const
    int32 bIsOuterTypeConst = clang_isConstQualifiedType(fieldCanonicalType);
    LOG("CppReflectionParser", "Field %s - Is const? %s", fieldName, !!(bIsOuterTypeConst) ? "true" : "false");

    // Inner type will be different in case of atomic type or pointer or array or vector or complex
    CXCursor innerTypeCursor = cursor;
    switch (fieldCanonicalType.kind)
    {
    case CXType_RValueReference:
        LOG("CppReflectionParser", "Field %s is a r-value", fieldName);
        break;
    case CXType_LValueReference:
        LOG("CppReflectionParser", "Field %s is a l-value", fieldName);
        break;
    case CXType_Pointer:
    {
        // Get cursor to declaration of pointer's type
        // Use declaration if only that type is not basic POD type, If POD then just inner type
        // will be same and child visitor will find the referenced type
        CXType innerType = clang_getPointeeType(fieldCanonicalType);
        if (!!clang_isPODType(fieldCanonicalType))
        {
            int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
            LOG("CppReflectionParser", "Field %s - Inner type %s is const? %s", fieldName, clang_getTypeSpelling(innerType),
                !!(bIsInnerTypeConst) ? "true" : "false");
        }
        else
        {
            innerTypeCursor = clang_getTypeDeclaration(innerType);
        }
        LOG("CppReflectionParser", "Field %s - pointer inner type is %s", fieldName, clang_getTypeSpelling(innerType));
        break;
    }
    case CXType_ConstantArray:
        LOG("CppReflectionParser", "Field %s - container element count %d", fieldName, clang_getNumElements(fieldType));
    case CXType_IncompleteArray:
    case CXType_DependentSizedArray:
    case CXType_Vector:
    case CXType_VariableArray:
    {
        // Use declaration if only that type is not basic POD type, If POD then just inner type
        // will be same and child visitor will find the referenced type
        CXType innerType = clang_getElementType(fieldCanonicalType);
        if (!!clang_isPODType(fieldCanonicalType))
        {
            int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
            LOG("CppReflectionParser", "Field %s - Element type %s is const? %s", fieldName, clang_getTypeSpelling(innerType),
                !!(bIsInnerTypeConst) ? "true" : "false");
        }
        else
        {
            innerTypeCursor = clang_getTypeDeclaration(innerType);
        }
        LOG("CppReflectionParser", "Field %s - container element type is %s", fieldName, clang_getTypeSpelling(innerType));
        break;
    }
    default:
        break;
    }
    CXType innerCanonicalType = fieldCanonicalType;
    if (!clang_equalCursors(innerTypeCursor, cursor))
    {
        // Get canonical cursor and find its type
        innerCanonicalType = clang_getCursorType(clang_getCanonicalCursor(innerTypeCursor));

        int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerCanonicalType);
        LOG("CppReflectionParser", "Field %s - Inner type %s is const? %s", fieldName, clang_getTypeSpelling(innerCanonicalType),
            !!(bIsInnerTypeConst) ? "true" : "false");
    }

    clang_visitChildren(
        innerTypeCursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
            switch (cursorKind)
            {
            case CXCursor_TypeRef:
            {
                // Just make sure we are using type alias's underlying canonical type
                // Cannot get canonical cursor here as POD will not be having any cursor, so use
                // canonical type instead CXCursor innerMostDeclTypeCursor =
                // clang_getCanonicalCursor(c);
                CXType innerMostType = clang_getCanonicalType(clang_getCursorType(c));
                LOG("CppReflectionParser", "printVariableTypeInfo() : Field's innermost canonical type is %s",
                    clang_getTypeSpelling(innerMostType));
                break;
            }
            case CXCursor_AnnotateAttr:
                break;
            default:
                CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo *)(clientData));
                break;
            }
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );
}

void printFunctionSignature(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    // This same can be obtained from CXType of function cursor using clang_getArgType for arg type at an
    // index in this function type clang_getResultType to find return type of this function type and
    // clang_getNumArgTypes to find total number of non template arguments

    CXType funcRetType = clang_getCursorResultType(cursor);
    int32 paramsCount = clang_Cursor_getNumArguments(cursor);
    std::vector<CXCursor> paramsCursor(paramsCount);
    for (uint32 i = 0; i < paramsCount; ++i)
    {
        paramsCursor[i] = clang_Cursor_getArgument(cursor, i);
    }

    String functionPath = String::join(srcParsedInfo.namespaceList.begin(), srcParsedInfo.namespaceList.end(), TCHAR("::"));
    CXStringRef functionName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    String functionParams;
    // print return type's param
    printJustTypeInfo(funcRetType);
    LOG("CppReflectionParser", "Return type unqualified name %s", ParserHelper::getNonConstTypeName(funcRetType, clang_getNullCursor()));
    {
        LOG("CppReflectionParser", "Function %s Arguments info ---->", functionName);
        std::vector<String> paramStrs;
        int32 i = 0;
        for (CXCursor c : paramsCursor)
        {
            String &paramDeclStr = paramStrs.emplace_back();
            CXType paramType = clang_getCursorType(c);
            CXStringRef paramTypeName(new CXStringWrapper(clang_getTypeSpelling(paramType)));
            CXStringRef paramName(new CXStringWrapper(clang_getCursorSpelling(c)));

            LOG("CppReflectionParser", "Argument %d Name %s Type %s(Unqualified %s)", i, paramName, paramTypeName,
                ParserHelper::getNonConstTypeName(paramType, c));
            printJustTypeInfo(paramType);

            paramDeclStr
                = UTF8_TO_TCHAR(clang_getCString(paramTypeName->str)) + String(TCHAR(" ")) + UTF8_TO_TCHAR(clang_getCString(paramName->str));
            ++i;
        }

        functionParams = String::join(paramStrs.cbegin(), paramStrs.cend(), TCHAR(", "));
    }

    LOG("CppReflectionParser", "Function %s Signature is %s %s::%s(%s)", functionName, clang_getTypeSpelling(funcRetType), functionPath,
        functionName, functionParams);
}

void visitTUCusor(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    switch (cursorKind)
    {
    // Declarations
    case CXCursor_UnexposedDecl:
        break;
    case CXCursor_StructDecl:
        visitStructs(cursor, srcParsedInfo);
        return;
    case CXCursor_UnionDecl:
        // TODO(Jeslas) : Add visit union function if needed
        visitStructs(cursor, srcParsedInfo);
        return;
    case CXCursor_ClassDecl:
        visitClasses(cursor, srcParsedInfo);
        return;
    case CXCursor_EnumDecl:
        visitEnums(cursor, srcParsedInfo);
        return;
    case CXCursor_FieldDecl:
        break;
    case CXCursor_EnumConstantDecl:
        break;
    case CXCursor_FunctionDecl:
        visitNonMemberFunctions(cursor, srcParsedInfo);
        return;
    case CXCursor_VarDecl:
        visitVariableDecl(cursor, srcParsedInfo);
        return;
    case CXCursor_ParmDecl:
        break;
    case CXCursor_ObjCInterfaceDecl:
        break;
    case CXCursor_ObjCCategoryDecl:
        break;
    case CXCursor_ObjCProtocolDecl:
        break;
    case CXCursor_ObjCPropertyDecl:
        break;
    case CXCursor_ObjCIvarDecl:
        break;
    case CXCursor_ObjCInstanceMethodDecl:
        break;
    case CXCursor_ObjCClassMethodDecl:
        break;
    case CXCursor_ObjCImplementationDecl:
        break;
    case CXCursor_ObjCCategoryImplDecl:
        break;
    case CXCursor_TypedefDecl:
        break;
    case CXCursor_CXXMethod:
        break;
    case CXCursor_Namespace:
        visitNameSpace(cursor, srcParsedInfo);
        return;
    case CXCursor_LinkageSpec:
        break;
    case CXCursor_Constructor:
        break;
    case CXCursor_Destructor:
        break;
    case CXCursor_ConversionFunction:
        break;
    // Parameters
    case CXCursor_TemplateTypeParameter:
        break;
    case CXCursor_NonTypeTemplateParameter:
        break;
    case CXCursor_TemplateTemplateParameter:
        break;
    // Templates
    case CXCursor_FunctionTemplate:
        break;
    case CXCursor_ClassTemplate:
        break;
    case CXCursor_ClassTemplatePartialSpecialization:
        break;
    // Alias, typedefs and using
    case CXCursor_NamespaceAlias:
        break;
    case CXCursor_UsingDirective:
        break;
    case CXCursor_UsingDeclaration:
        break;
    case CXCursor_TypeAliasDecl:
        break;
    // Obj C functions
    // case CXCursor_ObjCSynthesizeDecl:
    // case CXCursor_ObjCDynamicDecl:
    //    break;
    // Referring types or aliases
    case CXCursor_CXXAccessSpecifier:
        break;
    // Obj C functions
    // case CXCursor_ObjCSuperClassRef:
    // case CXCursor_ObjCProtocolRef:
    // case CXCursor_ObjCClassRef:
    //    break;
    case CXCursor_TypeRef:
        break;
    case CXCursor_CXXBaseSpecifier:
        break;
    case CXCursor_TemplateRef:
        break;
    case CXCursor_NamespaceRef:
        break;
    case CXCursor_MemberRef:
        break;
    case CXCursor_LabelRef:
        break;
    case CXCursor_OverloadedDeclRef:
        break;
    case CXCursor_VariableRef:
        break;
    // Errors or not found
    case CXCursor_InvalidFile:
        break;
    case CXCursor_NoDeclFound:
        break;
    case CXCursor_NotImplemented:
        break;
    case CXCursor_InvalidCode:
        break;
    // Expressions
    case CXCursor_UnexposedExpr:
        break;
    case CXCursor_DeclRefExpr:
        break;
    case CXCursor_MemberRefExpr:
        break;
    case CXCursor_CallExpr:
        break;
    case CXCursor_ObjCMessageExpr:
        break;
    case CXCursor_BlockExpr:
        break;
    // Literals, Expressions, Operators
    case CXCursor_IntegerLiteral:
        break;
    case CXCursor_FloatingLiteral:
        break;
    case CXCursor_ImaginaryLiteral:
        break;
    case CXCursor_StringLiteral:
        break;
    case CXCursor_CharacterLiteral:
        break;
    case CXCursor_ParenExpr:
        break;
    case CXCursor_UnaryOperator:
        break;
    case CXCursor_ArraySubscriptExpr:
        break;
    case CXCursor_BinaryOperator:
        break;
    case CXCursor_CompoundAssignOperator:
        break;
    case CXCursor_ConditionalOperator:
        break;
    case CXCursor_CStyleCastExpr:
        break;
    case CXCursor_CompoundLiteralExpr:
        break;
    case CXCursor_InitListExpr:
        break;
    case CXCursor_AddrLabelExpr:
        break;
    case CXCursor_StmtExpr:
        break;
    case CXCursor_GenericSelectionExpr:
        break;
    case CXCursor_GNUNullExpr:
        break;
    case CXCursor_CXXStaticCastExpr:
        break;
    case CXCursor_CXXDynamicCastExpr:
        break;
    case CXCursor_CXXReinterpretCastExpr:
        break;
    case CXCursor_CXXConstCastExpr:
        break;
    case CXCursor_CXXFunctionalCastExpr:
        break;
    case CXCursor_CXXTypeidExpr:
        break;
    case CXCursor_CXXBoolLiteralExpr:
        break;
    case CXCursor_CXXNullPtrLiteralExpr:
        break;
    case CXCursor_CXXThisExpr:
        break;
    case CXCursor_CXXThrowExpr:
        break;
    case CXCursor_CXXNewExpr:
        break;
    case CXCursor_CXXDeleteExpr:
        break;
    case CXCursor_UnaryExpr:
        break;
    case CXCursor_ObjCStringLiteral:
        break;
    case CXCursor_ObjCEncodeExpr:
        break;
    case CXCursor_ObjCSelectorExpr:
        break;
    case CXCursor_ObjCProtocolExpr:
        break;
    case CXCursor_ObjCBridgedCastExpr:
        break;
    case CXCursor_PackExpansionExpr:
        break;
    case CXCursor_SizeOfPackExpr:
        break;
    case CXCursor_LambdaExpr:
        break;
    case CXCursor_ObjCBoolLiteralExpr:
        break;
    case CXCursor_ObjCSelfExpr:
        break;
    case CXCursor_OMPArraySectionExpr:
        break;
    case CXCursor_ObjCAvailabilityCheckExpr:
        break;
    case CXCursor_FixedPointLiteral:
        break;
    case CXCursor_OMPArrayShapingExpr:
        break;
    case CXCursor_OMPIteratorExpr:
        break;
    case CXCursor_CXXAddrspaceCastExpr:
        break;
    // Statements
    case CXCursor_UnexposedStmt:
        break;
    case CXCursor_LabelStmt:
        break;
    case CXCursor_CompoundStmt:
        break;
    case CXCursor_CaseStmt:
        break;
    case CXCursor_DefaultStmt:
        break;
    case CXCursor_IfStmt:
        break;
    case CXCursor_SwitchStmt:
        break;
    case CXCursor_WhileStmt:
        break;
    case CXCursor_DoStmt:
        break;
    case CXCursor_ForStmt:
        break;
    case CXCursor_GotoStmt:
        break;
    case CXCursor_IndirectGotoStmt:
        break;
    case CXCursor_ContinueStmt:
        break;
    case CXCursor_BreakStmt:
        break;
    case CXCursor_ReturnStmt:
        break;
    case CXCursor_AsmStmt:
        break;
    // Obj C functions
    // case CXCursor_ObjCAtTryStmt:
    // case CXCursor_ObjCAtCatchStmt:
    // case CXCursor_ObjCAtFinallyStmt:
    // case CXCursor_ObjCAtThrowStmt:
    // case CXCursor_ObjCAtSynchronizedStmt:
    // case CXCursor_ObjCAutoreleasePoolStmt:
    // case CXCursor_ObjCForCollectionStmt:
    //    break;
    case CXCursor_CXXCatchStmt:
        break;
    case CXCursor_CXXTryStmt:
        break;
    case CXCursor_CXXForRangeStmt:
        break;
    case CXCursor_SEHTryStmt:
        break;
    case CXCursor_SEHExceptStmt:
        break;
    case CXCursor_SEHFinallyStmt:
        break;
    case CXCursor_MSAsmStmt:
        break;
    case CXCursor_NullStmt:
        break;
    case CXCursor_DeclStmt:
        break;
    case CXCursor_OMPParallelDirective:
        break;
    case CXCursor_OMPSimdDirective:
        break;
    case CXCursor_OMPForDirective:
        break;
    case CXCursor_OMPSectionsDirective:
        break;
    case CXCursor_OMPSectionDirective:
        break;
    case CXCursor_OMPSingleDirective:
        break;
    case CXCursor_OMPParallelForDirective:
        break;
    case CXCursor_OMPParallelSectionsDirective:
        break;
    case CXCursor_OMPTaskDirective:
        break;
    case CXCursor_OMPMasterDirective:
        break;
    case CXCursor_OMPCriticalDirective:
        break;
    case CXCursor_OMPTaskyieldDirective:
        break;
    case CXCursor_OMPBarrierDirective:
        break;
    case CXCursor_OMPTaskwaitDirective:
        break;
    case CXCursor_OMPFlushDirective:
        break;
    case CXCursor_SEHLeaveStmt:
        break;
    case CXCursor_OMPOrderedDirective:
        break;
    case CXCursor_OMPAtomicDirective:
        break;
    case CXCursor_OMPForSimdDirective:
        break;
    case CXCursor_OMPParallelForSimdDirective:
        break;
    case CXCursor_OMPTargetDirective:
        break;
    case CXCursor_OMPTeamsDirective:
        break;
    case CXCursor_OMPTaskgroupDirective:
        break;
    case CXCursor_OMPCancellationPointDirective:
        break;
    case CXCursor_OMPCancelDirective:
        break;
    case CXCursor_OMPTargetDataDirective:
        break;
    case CXCursor_OMPTaskLoopDirective:
        break;
    case CXCursor_OMPTaskLoopSimdDirective:
        break;
    case CXCursor_OMPDistributeDirective:
        break;
    case CXCursor_OMPTargetEnterDataDirective:
        break;
    case CXCursor_OMPTargetExitDataDirective:
        break;
    case CXCursor_OMPTargetParallelDirective:
        break;
    case CXCursor_OMPTargetParallelForDirective:
        break;
    case CXCursor_OMPTargetUpdateDirective:
        break;
    case CXCursor_OMPDistributeParallelForDirective:
        break;
    case CXCursor_OMPDistributeParallelForSimdDirective:
        break;
    case CXCursor_OMPDistributeSimdDirective:
        break;
    case CXCursor_OMPTargetParallelForSimdDirective:
        break;
    case CXCursor_OMPTargetSimdDirective:
        break;
    case CXCursor_OMPTeamsDistributeDirective:
        break;
    case CXCursor_OMPTeamsDistributeSimdDirective:
        break;
    case CXCursor_OMPTeamsDistributeParallelForSimdDirective:
        break;
    case CXCursor_OMPTeamsDistributeParallelForDirective:
        break;
    case CXCursor_OMPTargetTeamsDirective:
        break;
    case CXCursor_OMPTargetTeamsDistributeDirective:
        break;
    case CXCursor_OMPTargetTeamsDistributeParallelForDirective:
        break;
    case CXCursor_OMPTargetTeamsDistributeParallelForSimdDirective:
        break;
    case CXCursor_OMPTargetTeamsDistributeSimdDirective:
        break;
    case CXCursor_BuiltinBitCastExpr:
        break;
    case CXCursor_OMPMasterTaskLoopDirective:
        break;
    case CXCursor_OMPParallelMasterTaskLoopDirective:
        break;
    case CXCursor_OMPMasterTaskLoopSimdDirective:
        break;
    case CXCursor_OMPParallelMasterTaskLoopSimdDirective:
        break;
    case CXCursor_OMPParallelMasterDirective:
        break;
    case CXCursor_OMPDepobjDirective:
        break;
    case CXCursor_OMPScanDirective:
        break;
    case CXCursor_OMPTileDirective:
        break;
    case CXCursor_OMPCanonicalLoop:
        break;
    case CXCursor_OMPInteropDirective:
        break;
    case CXCursor_OMPDispatchDirective:
        break;
    case CXCursor_OMPMaskedDirective:
        break;
    case CXCursor_OMPUnrollDirective:
        break;
    // TU
    case CXCursor_TranslationUnit:
        break;
    // Attributes defined by __attribute__
    case CXCursor_UnexposedAttr:
        break;
    case CXCursor_IBActionAttr:
        break;
    case CXCursor_IBOutletAttr:
        break;
    case CXCursor_IBOutletCollectionAttr:
        break;
    case CXCursor_CXXFinalAttr:
        break;
    case CXCursor_CXXOverrideAttr:
        break;
    case CXCursor_AnnotateAttr:
        break;
    case CXCursor_AsmLabelAttr:
        break;
    case CXCursor_PackedAttr:
        break;
    case CXCursor_PureAttr:
        break;
    case CXCursor_ConstAttr:
        break;
    case CXCursor_NoDuplicateAttr:
        break;
    case CXCursor_CUDAConstantAttr:
        break;
    case CXCursor_CUDADeviceAttr:
        break;
    case CXCursor_CUDAGlobalAttr:
        break;
    case CXCursor_CUDAHostAttr:
        break;
    case CXCursor_CUDASharedAttr:
        break;
    case CXCursor_VisibilityAttr:
        break;
    case CXCursor_DLLExport:
        break;
    case CXCursor_DLLImport:
        break;
    case CXCursor_NSReturnsRetained:
        break;
    case CXCursor_NSReturnsNotRetained:
        break;
    case CXCursor_NSReturnsAutoreleased:
        break;
    case CXCursor_NSConsumesSelf:
        break;
    case CXCursor_NSConsumed:
        break;
    case CXCursor_ObjCException:
        break;
    case CXCursor_ObjCNSObject:
        break;
    case CXCursor_ObjCIndependentClass:
        break;
    case CXCursor_ObjCPreciseLifetime:
        break;
    case CXCursor_ObjCReturnsInnerPointer:
        break;
    case CXCursor_ObjCRequiresSuper:
        break;
    case CXCursor_ObjCRootClass:
        break;
    case CXCursor_ObjCSubclassingRestricted:
        break;
    case CXCursor_ObjCExplicitProtocolImpl:
        break;
    case CXCursor_ObjCDesignatedInitializer:
        break;
    case CXCursor_ObjCRuntimeVisible:
        break;
    case CXCursor_ObjCBoxable:
        break;
    case CXCursor_FlagEnum:
        break;
    case CXCursor_ConvergentAttr:
        break;
    case CXCursor_WarnUnusedAttr:
        break;
    case CXCursor_WarnUnusedResultAttr:
        break;
    case CXCursor_AlignedAttr:
        break;
    // Macro and preprocessors
    case CXCursor_PreprocessingDirective:
        break;
    case CXCursor_MacroDefinition:
        visitMacroDefinition(cursor, srcParsedInfo);
        return;
    case CXCursor_MacroExpansion:
        visitMacroExpansion(cursor, srcParsedInfo);
        return;
    case CXCursor_InclusionDirective:
        visitIncludes(cursor, srcParsedInfo);
        return;
    // Additional special declarations
    case CXCursor_ModuleImportDecl:
        break;
    case CXCursor_TypeAliasTemplateDecl:
        break;
    case CXCursor_StaticAssert:
        break;
    case CXCursor_FriendDecl:
        break;
    default:
        break;
    }

    {
        CXStringRef cursorSpelling(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef cursorKindSpelling(new CXStringWrapper(clang_getCursorKindSpelling(cursorKind)));
        LOG("CppReflectionParser", "Cursor '%s' of kind '%s'", cursorSpelling, cursorKindSpelling);
    }
}

void visitNameSpace(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    // Since we just need namespace's name string alone
    CXStringRef namespaceName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef displayName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Namespace %s starts - Display name %s", namespaceName, displayName);
    srcParsedInfo.namespaceList.push_back(UTF8_TO_TCHAR(clang_getCString(namespaceName->str)));

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo *)(clientData));
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    srcParsedInfo.namespaceList.pop_back();
    LOG("CppReflectionParser", "Namespace %s ends", namespaceName);
}

void visitMacroDefinition(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    // Get cursor location and TU to get token at this location
    CXSourceLocation cursorSrcLoc = clang_getCursorLocation(cursor);
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

    CXToken *token = clang_getToken(tu, cursorSrcLoc);
    CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, *token)));
    CXStringRef macroName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    LOG("CppReflectionParser", "Macro %s defined as %s", macroName, tokenStr);

    // TODO(Jeslas) : Find how to get macro's value and arguments if the cursor is function like macro
    clang_disposeTokens(tu, token, 1);
}

void visitMacroExpansion(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    // Get cursor location and TU to get token at this location
    CXSourceLocation cursorSrcLoc = clang_getCursorLocation(cursor);
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

    CXToken *token = clang_getToken(tu, cursorSrcLoc);
    CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, *token)));
    CXStringRef macroName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    LOG("CppReflectionParser", "Macro %s expanded as %s", macroName, tokenStr);

    // TODO(Jeslas) : Find how to get macro's expanded value and arguments passed in if the cursor is
    // function like macro
    clang_disposeTokens(tu, token, 1);
}

void visitIncludes(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    // Gets include's resolved file. It will be null if not resolved
    CXFile includeFile = clang_getIncludedFile(cursor);
    // Include file text
    CXStringRef inclsName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    if (includeFile != nullptr)
    {
        // Resolved the file in disk and gives back resolved file path, Empty if file does not exists
        // anymore
        CXStringRef inclsFilePath(new CXStringWrapper(clang_File_tryGetRealPathName(includeFile)));
        if (std::strlen(clang_getCString(inclsFilePath->str)) == 0)
        {
            // Gives the cached resolved path and file name
            inclsFilePath = new CXStringWrapper(clang_getFileName(includeFile));
        }
        LOG("CppReflectionParser", "\"%s\" include file resolved from %s", inclsName, inclsFilePath);
    }
    else
    {
        srcParsedInfo.includes.emplace_back(UTF8_TO_TCHAR(clang_getCString(inclsName->str)));
        LOG_ERROR("CppReflectionParser", "\"%s\" include file could not be resolved", inclsName);
    }
}

void visitClasses(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    // Since class defines new namespace for declared variables
    CXStringRef className(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef classDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Class %s starts - Display name %s", className, classDispName);
    srcParsedInfo.namespaceList.push_back(UTF8_TO_TCHAR(clang_getCString(className->str)));
    String currAccessSpecifier = srcParsedInfo.scopeAccessSpecifier;
    srcParsedInfo.scopeAccessSpecifier = TCHAR("private");

    // Below statement also returns type name with namespace so we do not need to keep track of namespace
    // String classPathName = ParserHelper::getNonConstTypeName(clang_getCursorType(cursor), cursor);
    String classPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), TCHAR("::"));
    LOG("CppReflectionParser", "Class full path name %s", classPathName);

    int32 bIsAbstract = clang_CXXRecord_isAbstract(cursor);
    if (!!bIsAbstract)
    {
        LOG("CppReflectionParser", "Class %s is abstract", className);
    }

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CppReflectionParser::visitClassMember(c, *(SourceParsedInfo *)(clientData));
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    srcParsedInfo.scopeAccessSpecifier = currAccessSpecifier;
    srcParsedInfo.namespaceList.pop_back();
    LOG("CppReflectionParser", "Class %s ends", className);
}

void visitClassMember(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    CXCursorKind cursorKind = clang_getCursorKind(cursor);
    CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(cursor)));

    switch (cursorKind)
    {
    case CXCursor_CXXBaseSpecifier:
    {
        String access;
#if 0 // Use switch to find access? Both case works
            access = accessSpecifierStr(cursor);
#else // Use source range to read access specifier directly, Both case works
      // AccessSpecDecl - Source range from access specifier token to colon ':' token
        CXSourceRange accessSpecDeclRange = clang_getCursorExtent(cursor);

        CXToken *tokens;
        uint32 tokensCount;
        clang_tokenize(tu, accessSpecDeclRange, &tokens, &tokensCount);

        fatalAssertf(
            tokensCount > 1,
            "Tokens must be atleast 2(Got %d) in case of access specifiers 'public' and "
            "(':' or 'class/struct name')",
            tokensCount
        );
        CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[0])));
        access = UTF8_TO_TCHAR(clang_getCString(tokenStr->str));
#endif
        // Since we need just string to log now cursor spelling provides base
        // record(class/struct) name. If need to find cursor/type one approach is to use same
        // technique used in FriendDecl visitor below To check if base struct is virtual(To avoid
        // multiple inheritance of base type)
        int32 bIsBaseVirtual = clang_isVirtualBase(cursor);
        // To check if base type is abstract we need cursor of that type declaration, We get base
        // type by getting cursor's type
        int32 bIsBaseAbstract = clang_CXXRecord_isAbstract(clang_getTypeDeclaration(clang_getCursorType(cursor)));
        LOG("CppReflectionParser", "Inherited from %s(%s and %s) with %s access specifier", cursorName,
            (!!bIsBaseAbstract) ? TCHAR("Abstract")
                                : TCHAR("Non-Abstract"), (!!bIsBaseVirtual) ? TCHAR("Virtual") : TCHAR("Non-Virtual"), access);
        break;
    }
    case CXCursor_AnnotateAttr:
        // Cursor spelling contains content of annotation
        LOG("CppReflectionParser", "[Access : %s] Annotated as %s", srcParsedInfo.scopeAccessSpecifier, cursorName);
        break;
    case CXCursor_CXXAccessSpecifier:
    {
        String access;
#if 0 // Use switch to find access? Both case works
            access = accessSpecifierStr(cursor);
#else // Use source range to read access specifier directly, Both case works
      // AccessSpecDecl - Source range from access specifier token to colon ':' token
        CXSourceRange accessSpecDeclRange = clang_getCursorExtent(cursor);

        CXToken *tokens;
        uint32 tokensCount;
        clang_tokenize(tu, accessSpecDeclRange, &tokens, &tokensCount);

        for (uint32 i = 0; i < tokensCount; ++i)
        {
            CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[i])));
            const AChar *str = clang_getCString(tokenStr->str);
            if (strcmp(str, ":") != 0)
            {
                access += UTF8_TO_TCHAR(str);
            }
        }
#endif
        LOG("CppReflectionParser", "Previous access %s new access is %s", srcParsedInfo.scopeAccessSpecifier, access);
        srcParsedInfo.scopeAccessSpecifier = access;
        break;
    }
    case CXCursor_TypeAliasDecl:
    case CXCursor_TypedefDecl:
    {
        // clang_getCursorType(cursor) gives type def type while
        // clang_getTypedefDeclUnderlyingType(cursor) gives actual type that is being aliased
        CXType type = clang_getTypedefDeclUnderlyingType(cursor);
        // Since this typedef/using decl might be type alias by itself get its canonical type
        type = clang_getCanonicalType(type);
        LOG("CppReflectionParser", "%s type is being aliased as %s", clang_getTypeSpelling(type), cursorName);
        break;
    }
    case CXCursor_FriendDecl:
    {
        CppReflectionParser::visitClassFriendDecl(cursor, srcParsedInfo, tu);
        break;
    }
    case CXCursor_FieldDecl:
        visitMemberField(cursor, srcParsedInfo);
        break;
    case CXCursor_Constructor:
    case CXCursor_Destructor:
    case CXCursor_ConversionFunction:
    case CXCursor_CXXMethod:
        // All member functions including static member functions
        visitMemberCppMethods(cursor, srcParsedInfo);
        break;
    case CXCursor_VarDecl:
        visitVariableDecl(cursor, srcParsedInfo);
        break;
    default:
        CppReflectionParser::visitTUCusor(cursor, srcParsedInfo);
        break;
    }
}

void visitClassFriendDecl(CXCursor cursor, SourceParsedInfo &srcParsedInfo, CXTranslationUnit tu)
{
    // Cursor spelling or display name do not provide any information about who this friend is
    // Has no cursor type kind
    // Source range of FriendDecl however gives the entire declaration, And it can be obtained using
    // clang_getCursorExtent
    String friendDeclStr;
    CXStringRef friendedType;
    {
        // FriendDecl - Source range will be from friend token to token before ';'
        CXSourceRange friendDeclRange = clang_getCursorExtent(cursor);
        // To skip friend we get source location token as well and skip it alone, as getLocation() gives
        // where friend keyword ends
        CXSourceLocation friendEndLoc = clang_getCursorLocation(cursor);

        CXToken *tokens;
        uint32 tokensCount;
        clang_tokenize(tu, friendDeclRange, &tokens, &tokensCount);

        std::vector<String> tokensStr;
        tokensStr.reserve(tokensCount);
        bool friendTokenEnded = false;
        CXCursor friendTypeCursor = clang_getNullCursor();
        for (uint32 i = 0; i < tokensCount; ++i)
        {
            CXSourceLocation tokenLoc = clang_getTokenLocation(tu, tokens[i]);
            friendTokenEnded = friendTokenEnded || clang_equalLocations(friendEndLoc, tokenLoc);
            if (friendTokenEnded)
            {
                // Trying to find friended type's cursor
                CXCursor typeCursor = clang_getCursor(tu, tokenLoc);
                // Works only for friend types and not for functions/methods
                if (clang_Cursor_isNull(friendTypeCursor) && clang_getCursorKind(typeCursor) != CXCursorKind::CXCursor_FriendDecl
                    && !(!!clang_Cursor_isNull(typeCursor) || clang_isInvalid(clang_getCursorKind(typeCursor))))
                {
                    friendTypeCursor = typeCursor;
                    // Below reference to canonical type is not necessary as getting cursor
                    // type from reference cursor still prints correct type name However in
                    // case of typedef or using it still prints only alias names, So If we
                    // need canonical type we need to use below deref type and then find
                    // actual canonical type or use `clang_getCanonicalType(cursorType)`
                    //
                    // This will always be just a reference find the referenced type
                    // friendTypeCursor = clang_getCursorReferenced(friendTypeCursor);
                    //// Once TypeRef is converted to TypeDecl find if it is TypeDefNamedDecl
                    /// or TypeAliasDecl
                    // while (clang_getCursorKind(friendTypeCursor) ==
                    // CXCursorKind::CXCursor_TypedefDecl
                    //     || clang_getCursorKind(friendTypeCursor) ==
                    //     CXCursorKind::CXCursor_TypeAliasDecl)
                    //{
                    //     friendTypeCursor =
                    //     clang_getTypeDeclaration(clang_getTypedefDeclUnderlyingType(friendTypeCursor));
                    // }
                }

                CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[i])));
                tokensStr.emplace_back(UTF8_TO_TCHAR(clang_getCString(tokenStr->str)));
            }
        }
        friendDeclStr = String::join(tokensStr.cbegin(), tokensStr.cend(), TCHAR(" "));

        if (!clang_Cursor_isNull(friendTypeCursor))
        {
            // Even though above gives string we better settle for CXType
            CXStringRef typeName;
            CXType cursorType = clang_getCursorType(friendTypeCursor);
            cursorType = clang_getCanonicalType(cursorType);
            friendedType = CXStringRef(new CXStringWrapper(clang_getTypeSpelling(cursorType)));
        }
        clang_disposeTokens(tu, tokens, tokensCount);
    }
    LOG("CppReflectionParser", "[Access : %s] %s(%s) is a friend of class %s", srcParsedInfo.scopeAccessSpecifier, friendedType, friendDeclStr,
        srcParsedInfo.namespaceList.back());
}

void visitStructs(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    // Since struct defines new namespace for declared variables
    CXStringRef structName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef structDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Struct %s starts - Display name %s", structName, structDispName);
    srcParsedInfo.namespaceList.push_back(UTF8_TO_TCHAR(clang_getCString(structName->str)));
    String currAccessSpecifier = srcParsedInfo.scopeAccessSpecifier;
    srcParsedInfo.scopeAccessSpecifier = TCHAR("public");

    String structPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), TCHAR("::"));
    LOG("CppReflectionParser", "Struct full path name %s", structPathName);

    // Since cursor is struct declaration
    int32 bIsAbstract = clang_CXXRecord_isAbstract(cursor);
    if (!!bIsAbstract)
    {
        LOG("CppReflectionParser", "Struct %s is abstract", structName);
    }

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CppReflectionParser::visitStructMember(c, *(SourceParsedInfo *)(clientData));
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    srcParsedInfo.scopeAccessSpecifier = currAccessSpecifier;
    srcParsedInfo.namespaceList.pop_back();
    LOG("CppReflectionParser", "Struct %s ends", structName);
}

void visitStructMember(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    CXCursorKind cursorKind = clang_getCursorKind(cursor);
    CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(cursor)));

    switch (cursorKind)
    {
    case CXCursor_CXXBaseSpecifier:
    {
        String access;
#if 0 // Use switch to find access? Both case works
            access = accessSpecifierStr(cursor);
#else // Use source range to read access specifier directly, Both case works
      // AccessSpecDecl - Source range from access specifier token to colon ':' token
        CXSourceRange accessSpecDeclRange = clang_getCursorExtent(cursor);

        CXToken *tokens;
        uint32 tokensCount;
        clang_tokenize(tu, accessSpecDeclRange, &tokens, &tokensCount);

        fatalAssertf(
            tokensCount > 1,
            "Tokens must be atleast 2(Got %d) in case of access specifiers 'public' and "
            "(':' or 'class/struct name')",
            tokensCount
        );
        CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[0])));
        access = UTF8_TO_TCHAR(clang_getCString(tokenStr->str));
#endif
        // Since we need just string to log now cursor spelling provides base
        // record(class/struct) name. If need to find cursor/type one approach is to use same
        // technique used in FriendDecl visitor below To check if base struct is virtual(To avoid
        // multiple inheritance of base type)
        int32 bIsBaseVirtual = clang_isVirtualBase(cursor);
        // To check if base type is abstract we need cursor of that type declaration, We get base
        // type by getting cursor's type
        int32 bIsBaseAbstract = clang_CXXRecord_isAbstract(clang_getTypeDeclaration(clang_getCursorType(cursor)));
        LOG("CppReflectionParser", "Inherited from %s(%s and %s) with %s access specifier", cursorName,
            (!!bIsBaseAbstract) ? TCHAR("Abstract")
                                : TCHAR("Non-Abstract"), (!!bIsBaseVirtual) ? TCHAR("Virtual") : TCHAR("Non-Virtual"), access);
        break;
    }
    case CXCursor_AnnotateAttr:
        // Cursor spelling contains content of annotation
        LOG("CppReflectionParser", "[Access : %s] Annotated as %s", srcParsedInfo.scopeAccessSpecifier, cursorName);
        break;
    case CXCursor_TypeAliasDecl:
    case CXCursor_TypedefDecl:
    {
        // clang_getCursorType(cursor) gives type def type while
        // clang_getTypedefDeclUnderlyingType(cursor) gives actual type that is being aliased
        CXType type = clang_getTypedefDeclUnderlyingType(cursor);
        // Since this typedef/using decl might be type alias by itself get its canonical type
        type = clang_getCanonicalType(type);
        LOG("CppReflectionParser", "%s type is being aliased as %s", clang_getTypeSpelling(type), cursorName);
        break;
    }
    case CXCursor_FieldDecl:
        visitMemberField(cursor, srcParsedInfo);
        break;
    case CXCursor_CXXMethod:
        visitMemberCppMethods(cursor, srcParsedInfo);
        break;
    case CXCursor_VarDecl:
        visitVariableDecl(cursor, srcParsedInfo);
        break;
    default:
        CppReflectionParser::visitTUCusor(cursor, srcParsedInfo);
        break;
    }
}

void visitEnums(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXStringRef enumName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef enumDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Enum %s : Display name %s", enumName, enumDispName);
    srcParsedInfo.namespaceList.push_back(UTF8_TO_TCHAR(clang_getCString(enumName->str)));

    String enumPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), TCHAR("::"));
    LOG("CppReflectionParser", "Enum %s - Full path name %s", enumName, enumPathName);

    int32 bIsScopedEnum = clang_EnumDecl_isScoped(cursor);
    LOG("CppReflectionParser", "Enum %s : Is scoped enum(Strongly typed with Class)? %s", enumName,
        (!!bIsScopedEnum) ? TCHAR("true") : TCHAR("false"));

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
            CXStringRef enumName(new CXStringWrapper(clang_getCursorSpelling(p)));
            switch (cursorKind)
            {
            case CXCursor_AnnotateAttr:
                // Cursor spelling contains content of annotation
                LOG("CppReflectionParser", "visitEnums() : Enum %s - Annotated as %s", enumName, cursorName);
                break;
            case CXCursor_EnumConstantDecl:
            {
                int64 enumVal = clang_getEnumConstantDeclValue(c);
                LOG("CppReflectionParser", "visitEnums() : Enum %s - Value(name %s, value %ld)", enumName, cursorName, enumVal);
                return CXChildVisit_Recurse;
            }
            default:
                CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo *)(clientData));
                break;
            }
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    srcParsedInfo.namespaceList.pop_back();
}

void visitMemberField(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef fieldDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Field %s - Display name %s", fieldName, fieldDispName);

    String fieldPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), TCHAR("::"));
    LOG("CppReflectionParser", "Field %s - Base path name %s", fieldName, fieldPathName);

    CXType fieldType = clang_getCursorType(cursor);
    CXType fieldCanonicalType = clang_getCanonicalType(fieldType);
    CXStringRef typeKindName(new CXStringWrapper(clang_getTypeKindSpelling(fieldCanonicalType.kind)));
    CXStringRef canonicalTypeName(new CXStringWrapper(clang_getTypeSpelling(fieldCanonicalType)));
    CXStringRef typeName = canonicalTypeName;
    if (!clang_equalTypes(fieldType, fieldCanonicalType))
    {
        typeName = new CXStringWrapper(clang_getTypeSpelling(fieldType));
    }
    LOG("CppReflectionParser", "Field %s - Field typename %s, Canonical typename %s, Type kind %s", fieldName, typeName, canonicalTypeName,
        typeKindName);
    if (fieldType.kind == CXType_LValueReference)
    {
        LOG_ERROR("CppReflectionParser", "Field %s - Having reference member field is not good!", fieldName);
        return;
    }

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
            CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(p)));
            switch (cursorKind)
            {
            case CXCursor_AnnotateAttr:
                // Cursor spelling contains content of annotation
                LOG("CppReflectionParser", "visitMemberField() : Field %s - Annotated as %s", fieldName, cursorName);
                break;
            default:
                CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo *)(clientData));
                break;
            }
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    printVariableTypeInfo(cursor, srcParsedInfo, fieldType, fieldCanonicalType);
}

void visitVariableDecl(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXStringRef varName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef varDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Variable %s - Display name %s", varName, varDispName);

    String fieldPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), TCHAR("::"));
    LOG("CppReflectionParser", "Variable %s - Base path name %s", varName, fieldPathName);

    CXType fieldType = clang_getCursorType(cursor);
    CXType fieldCanonicalType = clang_getCanonicalType(fieldType);
    CXStringRef typeKindName(new CXStringWrapper(clang_getTypeKindSpelling(fieldCanonicalType.kind)));
    CXStringRef canonicalTypeName(new CXStringWrapper(clang_getTypeSpelling(fieldCanonicalType)));
    CXStringRef typeName = canonicalTypeName;
    if (!clang_equalTypes(fieldType, fieldCanonicalType))
    {
        typeName = new CXStringWrapper(clang_getTypeSpelling(fieldType));
    }
    LOG("CppReflectionParser", "Variable %s - Variable typename %s, Canonical typename %s, Type kind %s", varName, typeName, canonicalTypeName,
        typeKindName);

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
            CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(p)));
            switch (cursorKind)
            {
            case CXCursor_AnnotateAttr:
                // Cursor spelling contains content of annotation
                LOG("CppReflectionParser", "visitVariableDecl() : Field %s - Annotated as %s", fieldName, cursorName);
                break;
            default:
                CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo *)(clientData));
                break;
            }
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    printVariableTypeInfo(cursor, srcParsedInfo, fieldType, fieldCanonicalType);
}

void visitNonMemberFunctions(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef funcDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Function %s - Display name %s", funcName, funcDispName);

    String funcPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), TCHAR("::"));
    LOG("CppReflectionParser", "Function %s - Base path name %s", funcName, funcPathName);

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
            CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(p)));
            switch (cursorKind)
            {
            case CXCursor_AnnotateAttr:
                // Cursor spelling contains content of annotation
                LOG("CppReflectionParser", "visitNonMemberFunctions() : Function %s - Annotated as %s", funcName, cursorName);
                break;
            case CXCursor_ParmDecl:
                // Since we handle this in print function signature
                break;
            default:
                CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo *)(clientData));
                break;
            }
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    printFunctionSignature(cursor, srcParsedInfo);
}

void visitMemberCppMethods(CXCursor cursor, SourceParsedInfo &srcParsedInfo)
{
    CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
    CXStringRef funcDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
    LOG("CppReflectionParser", "Function %s - Display name %s", funcName, funcDispName);

    String funcPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), TCHAR("::"));
    LOG("CppReflectionParser", "Function %s - Base path name %s", funcName, funcPathName);

    int32 bIsPureVirtual = clang_CXXMethod_isPureVirtual(cursor);
    int32 bIsVirtual = clang_CXXMethod_isVirtual(cursor);
    int32 bIsStatic = clang_CXXMethod_isStatic(cursor);
    int32 bIsConst = clang_CXXMethod_isConst(cursor);
    LOG("CppReflectionParser", "Function %s - %s%s", funcName,
        (!!bIsStatic) ? TCHAR("Static and ")
                      : (!!bIsConst)
        ? TCHAR("Const and ")
        : TCHAR(""), (!!bIsVirtual) ? (!!bIsPureVirtual) ? TCHAR("Pure virtual") : TCHAR("virtual") : TCHAR("Non-virtual"));
    // If not pure virtual method then print all base methods
    if (bIsVirtual && !bIsPureVirtual)
    {
        uint32 numOverrides;
        CXCursor *baseCursors;
        // Gives up-to only one level
        clang_getOverriddenCursors(cursor, &baseCursors, &numOverrides);
        uint32 levelFromThisOverride = 1;
        LOG("CppReflectionParser", "Function %s - Overrides following methods ---->", funcName);
        std::vector<ArrayView<CXCursor>> currOverridenCursors{ ArrayView<CXCursor>(baseCursors, numOverrides) };
        while (!currOverridenCursors.empty())
        {
            std::vector<ArrayView<CXCursor>> newOverridenCursors;
            for (ArrayView<CXCursor> &overridenCursors : currOverridenCursors)
            {
                for (uint32 i = 0; i < overridenCursors.size(); ++i)
                {
                    // Get the class that this overridden method's cursor belongs to by
                    // getting type of the method and getting the type's class type Below
                    // does not provider parent class as cursor is not member pointer
                    // type CXType overridenClassType =
                    // clang_Type_getClassType(clang_getCursorType(overridenCursors[i]));
                    // Semantic parent gives place where this cursor is declared in
                    CXType overridenClassType = clang_getCursorType(clang_getCursorSemanticParent(overridenCursors[i]));
                    LOG("CppReflectionParser", "Function %s - (Level %d) method %s of %s", funcName, levelFromThisOverride,
                        clang_getCursorSpelling(overridenCursors[i]), clang_getTypeSpelling(overridenClassType));

                    // Add overrides of this base class's function
                    numOverrides = 0;
                    baseCursors = nullptr;
                    clang_getOverriddenCursors(overridenCursors[i], &baseCursors, &numOverrides);
                    if (numOverrides > 0)
                    {
                        newOverridenCursors.emplace_back(ArrayView<CXCursor>(baseCursors, numOverrides));
                    }
                }
                // Now dispose this cursors
                clang_disposeOverriddenCursors(overridenCursors.data());
            }
            levelFromThisOverride++;
            currOverridenCursors = std::move(newOverridenCursors);
        }
    }

    // The caller's reference type(lvalue or rvalue) else if no valid values then pointer or lvalue that
    // calls this overload of the function If none `retType func(Params...)` or `retType func(Params...)
    // const` are allowed overloads and corresponding function gets called If lvalue `retType
    // func(Params...) &` or `retType func(Params...) const &` gets called If rvalue `retType
    // func(Params...) &&` gets called
    CXRefQualifierKind methodCalledRefKind = clang_Type_getCXXRefQualifier(clang_getCursorType(cursor));
    if (methodCalledRefKind != CXRefQualifier_None)
    {
        LOG("CppReflectionParser", "Function %s can be called from %s-value reference only", funcName,
            (methodCalledRefKind == CXRefQualifier_LValue) ? TCHAR("l") : TCHAR("r"));
    }

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData clientData)
        {
            CXCursorKind cursorKind = clang_getCursorKind(c);
            CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
            CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(p)));
            switch (cursorKind)
            {
            case CXCursor_CXXFinalAttr:
                LOG("CppReflectionParser", "visitMemberCppMethods() : Function %s - virtual is made final", funcName);
                break;
            case CXCursor_CXXOverrideAttr:
                LOG("CppReflectionParser", "visitMemberCppMethods() : Function %s - Has attribute override", funcName);
                break;
            case CXCursor_AnnotateAttr:
                // Cursor spelling contains content of annotation
                LOG("CppReflectionParser", "visitMemberCppMethods() : Function %s - Annotated as %s", funcName, cursorName);
                break;
            case CXCursor_ParmDecl:
                // Since we handle this in print function signature
                break;
            default:
                CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo *)(clientData));
                break;
            }
            return CXChildVisit_Continue;
        },
        &srcParsedInfo
    );

    printFunctionSignature(cursor, srcParsedInfo);
}

} // namespace CppReflectionParser

namespace SampleCode
{
void testLibClangParsing(String srcDir) noexcept
{
    CXIndex index = clang_createIndex(0, 0);
    std::string argRefParseDef("-D__REF_PARSE__");
    std::string argIncludeModulePublic("-ID:/Workspace/VisualStudio/Cranberry/Source/Runtime/ProgramCore/Public");
    std::string argIncludeModuleGen("-ID:/Workspace/VisualStudio/Cranberry/Source/Runtime/ProgramCore/Generated/Public");
    const AChar *args[] = { argIncludeModuleGen.c_str(), argIncludeModulePublic.c_str(), argRefParseDef.c_str() };
    // Use parse TU functions if need to customize certain options while compiling
    // Header.H - H has to be capital but why?
    // It is okay if we miss some insignificant includes as they are ignored and parsing continues
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        TCHAR_TO_ANSI(PathFunctions::combinePath(srcDir, TCHAR("SampleHeader.H")).getChar()), args, 3, nullptr, 0, CXTranslationUnit_KeepGoing
        );
    // CXTranslationUnit unit = clang_createTranslationUnitFromSourceFile(index
    //     , PathFunctions::combinePath(srcDir, "Header.H").getChar()
    //     , 3, args, 0, nullptr);
    if (unit == nullptr)
    {
        LOG_ERROR("TestCode", "Unable to parse translation unit. Quitting.");
        return;
    }
    else
    {
        uint32 formatOptions
            = CXDiagnostic_DisplaySourceLocation | CXDiagnostic_DisplayColumn | CXDiagnostic_DisplayCategoryName | CXDiagnostic_DisplayOption;
        uint32 diagnosticsNum = clang_getNumDiagnostics(unit);
        for (uint32 i = 0; i < diagnosticsNum; ++i)
        {
            auto diagnostic = clang_getDiagnostic(unit, i);
            CppReflectionParser::printDiagnostics(diagnostic, formatOptions);
            clang_disposeDiagnostic(diagnostic);
        }
    }

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    CppReflectionParser::SourceParsedInfo parsedInfo;
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data)
        {
            // If this symbol is from this Source file?
            // CXSourceLocation is not need to be freed
            if (!!clang_Location_isFromMainFile(clang_getCursorLocation(c)))
            {
                CppReflectionParser::visitTUCusor(c, *(CppReflectionParser::SourceParsedInfo *)(client_data));
            }
            // Continue to next Cursor in TU
            return CXChildVisit_Continue;
        },
        &parsedInfo
    );

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
}

struct TestDataProperties
{
    int32 normalInt;
    String normalString;
    int32 *intPtr = nullptr;
    const int32 *constIntPtr = nullptr;

    static String staticVal;

    void modifyValues(int32 a)
    {
        normalInt *= a;
        normalString = TCHAR("Modified by func ") + normalString;
        staticVal = TCHAR("Modified by func ") + staticVal;
    }
};
String TestDataProperties::staticVal = TCHAR("Hello World");
String globalVal;
int32 globalMod(int32 &reminder, int32 dividend, int32 divisor)
{
    int32 quotient = Math::floor(dividend / (float)divisor);
    reminder = dividend - (quotient * divisor);
    return quotient;
}

void testTypesAndProperties()
{
    LOG("Test",
        "Test type info \n%s\n%s\n%s\n%s\n%s\n%s\n%s"
        // Referenced variable is const
        ,
        *typeInfoFrom<const int32 &>(),
        *typeInfoFrom<const int32 &&>()
        // Pointer to const variable
        ,
        *typeInfoFrom<const int32 *>()
        // Const pointer to const variable
        ,
        *typeInfoFrom<int32 const *const>()
        // Const reference to pointer to const variable
        ,
        *typeInfoFrom<int32 const *const &>(), *typeInfoFrom<int32 const *const &&>(), *typeInfoFrom<std::vector<int32> &>());
    LOG("Test", "Test type info %d, %d, %d", *typeInfoFrom<const int32 &>() == *typeInfoFrom<const int32 &&>(),
        *typeInfoFrom<const int32 *>() == *typeInfoFrom<int32 const *const &>(),
        *typeInfoFrom<int32 const *const &>() == *typeInfoFrom<int32 const *const &&>());

    auto testList
        = typeInfoListFrom<const int32 &, const int32 &&, const int32 *, int32 const *const, int32 const *const &, int32 const *const &&>();

    TestDataProperties testDataProps;
    int32 tempVal = 1;
    int32 tempVal2 = 5;
    MemberFieldWrapperImpl<TestDataProperties, int32> normalIntProp(&TestDataProperties::normalInt);
    MemberFieldWrapperImpl<TestDataProperties, String> normalStrProp(&TestDataProperties::normalString);
    MemberFieldWrapperImpl<TestDataProperties, int32 *> intPtrProp(&TestDataProperties::intPtr);
    MemberFieldWrapperImpl<TestDataProperties, const int32 *> constIntPtrProp(&TestDataProperties::constIntPtr);
    GlobalFieldWrapperImpl<String> staticValProp(&TestDataProperties::staticVal);
    GlobalFieldWrapperImpl<String> globalValProp(&SampleCode::globalVal);

    LOG("Test",
        "Before setting values : \n    normalInt %d\n    normatString %s\n    intPtr 0x%llx(%d)\n    "
        "constIntPtr 0x%llx(%d)\n    staticVal %s\n    globalVal %s",
        *normalIntProp.getAsType<int32>(testDataProps).vPtr, *normalStrProp.getAsType<String>(testDataProps).vPtr,
        (intPtrProp.getAsType<int32 *>(testDataProps)) ? reinterpret_cast<uint64>(*intPtrProp.getAsType<int32 *>(testDataProps).vPtr) : 0,
        (intPtrProp.getAsType<int32 *>(testDataProps) && *intPtrProp.getAsType<int32 *>(testDataProps).vPtr)
            ? **intPtrProp.getAsType<int32 *>(testDataProps).vPtr
            : 0,
        constIntPtrProp.getAsType<const int32 *>(testDataProps)
            ? reinterpret_cast<uint64>(*constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr)
            : 0,
        (constIntPtrProp.getAsType<const int32 *>(testDataProps) && *constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr)
            ? **constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr
            : 0,
        *staticValProp.getAsType<String>().vPtr, *globalValProp.getAsType<String>().vPtr);

    normalIntProp.setFromType<int32>(28u, &testDataProps);
    normalIntProp.setFromType(TCHAR("test"), &testDataProps);
    normalStrProp.setFromType<String>(TCHAR("Hello this is normal str"), testDataProps);
    intPtrProp.setFromType(&tempVal, testDataProps);
    constIntPtrProp.setFromType(&tempVal, testDataProps);
    **intPtrProp.getAsType<int32 *>(testDataProps).vPtr = 9;
    if (auto ptrToProp = constIntPtrProp.getAsType<int32 *>(testDataProps))
    {
        **ptrToProp.vPtr = 10;
    }
    else
    {
        *constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr = &tempVal2;
    }
    staticValProp.setFromType<String>(TCHAR("This is static"));
    globalValProp.setFromType<String>(TCHAR("This is global static"));

    LOG("Test",
        "After setting values : \n    normalInt %d\n    normatString %s\n    intPtr 0x%llx(%d)\n    "
        "constIntPtr 0x%llx(%d)\n    staticVal %s\n    globalVal %s",
        *normalIntProp.getAsType<int32>(testDataProps).vPtr, *normalStrProp.getAsType<String>(testDataProps).vPtr,
        (intPtrProp.getAsType<int32 *>(testDataProps)) ? reinterpret_cast<uint64>(*intPtrProp.getAsType<int32 *>(testDataProps).vPtr) : 0,
        (intPtrProp.getAsType<int32 *>(testDataProps) && *intPtrProp.getAsType<int32 *>(testDataProps).vPtr)
            ? **intPtrProp.getAsType<int32 *>(testDataProps).vPtr
            : 0,
        constIntPtrProp.getAsType<const int32 *>(testDataProps)
            ? reinterpret_cast<uint64>(*constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr)
            : 0,
        (constIntPtrProp.getAsType<const int32 *>(testDataProps) && *constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr)
            ? **constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr
            : 0,
        *staticValProp.getAsType<String>().vPtr, *globalValProp.getAsType<String>().vPtr);

    MemberFunctionWrapperImpl<TestDataProperties, void, int32> modifierFunc(&TestDataProperties::modifyValues);
    GlobalFunctionWrapperImpl<int32, int32 &, int32, int32> modFunc(&globalMod);

    modifierFunc.invokeVoid(testDataProps, 34);
    LOG("Test",
        "After Modify values : \n    normalInt %d\n    normatString %s\n    intPtr 0x%llx(%d)\n    "
        "constIntPtr 0x%llx(%d)\n    staticVal %s\n    globalVal %s",
        *normalIntProp.getAsType<int32>(testDataProps).vPtr, *normalStrProp.getAsType<String>(testDataProps).vPtr,
        (intPtrProp.getAsType<int32 *>(testDataProps)) ? reinterpret_cast<uint64>(*intPtrProp.getAsType<int32 *>(testDataProps).vPtr) : 0,
        (intPtrProp.getAsType<int32 *>(testDataProps) && *intPtrProp.getAsType<int32 *>(testDataProps).vPtr)
            ? **intPtrProp.getAsType<int32 *>(testDataProps).vPtr
            : 0,
        constIntPtrProp.getAsType<const int32 *>(testDataProps)
            ? reinterpret_cast<uint64>(*constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr)
            : 0,
        (constIntPtrProp.getAsType<const int32 *>(testDataProps) && *constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr)
            ? **constIntPtrProp.getAsType<const int32 *>(testDataProps).vPtr
            : 0,
        *staticValProp.getAsType<String>().vPtr, *globalValProp.getAsType<String>().vPtr);

    int32 q = 0;
    int32 r = 0;
    modFunc.invoke(q, r, 4, 3);
    LOG("Test", "Quotient %d, Remainder %d, Dividend %d, Divisor %d", q, r, 4, 3);
}

void testRegex()
{
    String testStr = TCHAR("\n\
        Hello {{name}}, \n\
        {{ \n\
        This must match {{Match2}} \n\
            {\n\
                {{name}}{{{{HelloMe}}}}\n\
                {{#PrintInner}} \
                This is inner code for {{name}}\
                {{/PrintInner}} \n\
            }\n\
        }};\n");
    String testStr2 = TCHAR("This is going to be used as partial \n\
            Peoples Details :{{!List of peoples}}\n\
            {{#Run}} \
                {{>Peps}} \
            {{/Run}} \
        ");
#if 0 // Prints the regex matched values and its indices
        std::vector<std::smatch> allMatches;
        StringRegex searchPattern(TCHAR("\\{\\{([^{}]+)\\}\\}"), std::regex_constants::ECMAScript);

        auto startItr = testStr.cbegin();
        StringMatch matches;
        while (std::regex_search(startItr, testStr.cend(), matches, searchPattern))
        {
            allMatches.push_back(matches);
            LOG("Test", "Matched n = %d", matches.size());
            LOG("Test", "Prefix : %s", matches.prefix().str());
            for (auto& match : matches)
            {
                LOG("Test", "Match : %s", match.str());
            }
            LOG("Test", "suffix : %s", matches.suffix().str());
            startItr = matches.suffix().first;
        }

        for (const StringMatch& match : allMatches)
        {
            const StringSubmatch& submatch = match[match.size() - 1];
            const StringSubmatch& wholesubmatch = match[0];
            LOG("Test", "Must replace with value of %s, from index %d to %d"
                , submatch.str(), std::distance(testStr.cbegin(), wholesubmatch.first), std::distance(testStr.cbegin(), wholesubmatch.second));
        }
#endif

    std::unordered_map<String, FormatArg> args{
        {      TCHAR("name"), TCHAR("Jeslas Pravin")},
        {    TCHAR("Match2"),                   8235},
        {   TCHAR("HelloMe"),                 123.08},
        {TCHAR("PrintInner"),                  false}
    };
    std::unordered_map<String, FormatArg> args2{
        {      TCHAR("name"),  TCHAR("Subity Jerald")},
          {    TCHAR("Match2"),                    8265},
        {   TCHAR("HelloMe"), *typeInfoFrom<uint32>()},
        {TCHAR("PrintInner"),                    true}
    };
    MustacheStringFormatter peps{ testStr };
    MustacheStringFormatter mustacheTest{ testStr2 };
    LOG("Test", "Mustache formatted \n%s \n\tand another \n%s", peps.formatBasic(args), peps.formatBasic(args2));

    MustacheContext context1{ .sectionContexts = { { TCHAR("Run"), { MustacheContext{ .args = args }, MustacheContext{ .args = args2 } } } } };
    LOG("Test", "Mustache rendered \n%s",
        mustacheTest.render(
            context1,
            {
                {TCHAR("Peps"), peps}
    }
            ));

    String testStr3 = TCHAR("ID : {{Count}}{{#MSectFormat}}{{!This will be replaced}}\
        {{#CanRecurse}}\n{{>Recurse}}\n{{/CanRecurse}}{{/MSectFormat}}");
    MustacheStringFormatter sectFormatter{ testStr3 };
    class TestDynamicFormatData
    {
    public:
        MustacheStringFormatter *localFormatter;
        const FormatArgsMap *arg1;
        const FormatArgsMap *arg2;
        int32 count = 0;

        String customFormat(
            const MustacheStringFormatter &formatter, const MustacheContext &context,
            const std::unordered_map<String, MustacheStringFormatter> &partials
        )
        {
            String out = localFormatter->render({ (count % 2) == 0 ? *arg1 : *arg2 }, partials);
            count++;
            out += formatter.render(context, partials);
            return out;
        }

        String getCount() const { return String::toString(count); }

        String canRecurse() const
        {
            if (count < 10)
            {
                return String::toString(count);
            }
            return {};
        }
    };
    TestDynamicFormatData dynDataTest{ &peps, &args, &args2 };
    MustacheContext context2{
        .args = {       { TCHAR("CanRecurse"), FormatArg::ArgGetter::createObject(&dynDataTest, &TestDynamicFormatData::canRecurse) },
                 { TCHAR("Count"), FormatArg::ArgGetter::createObject(&dynDataTest, &TestDynamicFormatData::getCount) } },
                  .sectionFormatters
                  = {{ TCHAR("MSectFormat"), MustacheSectionFormatter::createObject(&dynDataTest, &TestDynamicFormatData::customFormat) } }
    };
    LOG("Test", "Mustache render dynamically modified recursive loop \n%s",
        sectFormatter.render(
            context2,
            {
                {TCHAR("Recurse"), sectFormatter}
    }
            ));

    LOG("Test", "%s", PropertyHelper::getValidSymbolName(TCHAR("class <Niown>>")));
}

class TestPropertyClass
{
public:
    struct TestInnerStruct
    {
        std::vector<String> names;
        uint32 numNames;
    };

    enum ETestEnumType
    {
        E1 = 1,
        E2,
        E3,
        E4
    };

public:
    std::map<int32, TestInnerStruct> idToSection;
    String newNameStr;

    TestPropertyClass *nextClass;
    std::set<uint64> handles;

    static int32 staticInteger;

public:
    TestPropertyClass(String newName)
        : newNameStr(newName)
    {}

    void printNewNameStr() const { LOG("TestPropertyClass", "New Name str %s", newNameStr); }

    void setNewNameStr(const String &newName) { newNameStr = newName; }

    static void incAndPrintInt()
    {
        staticInteger++;
        LOG("TestPropertyClass", "New int value %d", staticInteger);
    }
};
int32 TestPropertyClass::staticInteger = 8235;

struct RegisterPropertyFactory_TestPropertyClass
{
    using ThisType = RegisterPropertyFactory_TestPropertyClass;
    RegisterPropertyFactory_TestPropertyClass()
    {
        // Just for testing, Since only runtime game/engine modules can have reflected data
        // Tools do not have reflection
        ModuleManager::get()->loadModule(TCHAR("ReflectionRuntime"));

        IReflectionRuntimeModule::get()->registerTypeFactory(
            typeInfoFrom<TestPropertyClass *>(), { &ThisType::createTestPropertyClassPtrProperty, &ThisType::initTestPropertyClassPtrProperty }
        );
        IReflectionRuntimeModule::get()->registerTypeFactory(
            typeInfoFrom<std::pair<const int32, TestPropertyClass::TestInnerStruct>>(),
            { &ThisType::createstd__pair_const_int32__TestPropertyClass__TestInnerStruct_Property,
              &ThisType::initstd__pair_const_int32__TestPropertyClass__TestInnerStruct_Property }
        );
        IReflectionRuntimeModule::get()->registerTypeFactory(
            typeInfoFrom<std::map<int32, TestPropertyClass::TestInnerStruct>>(),
            { &ThisType::createstd__map_int32__TestPropertyClass__TestInnerStruct_Property,
              &ThisType::initstd__map_int32__TestPropertyClass__TestInnerStruct_Property }
        );
        IReflectionRuntimeModule::get()->registerTypeFactory(
            typeInfoFrom<std::set<uint64>>(), { &ThisType::createstd__set_uint64_Property, &ThisType::initstd__set_uint64_Property }
        );

        IReflectionRuntimeModule::get()->registerClassFactory(
            STRID("TestPropertyClass"), typeInfoFrom<TestPropertyClass>(), { &ThisType::createTestPropertyClassProperty, &ThisType::initTestPropertyClassProperty }
        );
    }

    static BaseProperty *createTestPropertyClassPtrProperty()
    {
        QualifiedProperty *prop
            = (new QualifiedProperty(STRID("TestPropertyClass*"), TCHAR("TestPropertyClass*"), typeInfoFrom<TestPropertyClass *>()));
        return prop;
    }
    static void initTestPropertyClassPtrProperty(BaseProperty *prop)
    {
        QualifiedProperty *p = static_cast<QualifiedProperty *>(prop);
        p->setUnqualifiedType(IReflectionRuntimeModule::getClassType<TestPropertyClass>());
    }

    static BaseProperty *createstd__pair_const_int32__TestPropertyClass__TestInnerStruct_Property()
    {
        BaseProperty *prop = new PairProperty(
            STRID("std::pair<const int32, TestPropertyClass::TestInnerStruct>"), TCHAR("std::pair<const int32, TestPropertyClass::TestInnerStruct>"), typeInfoFrom<std::pair<const int32, TestPropertyClass::TestInnerStruct>>());
        return prop;
    }
    static void initstd__pair_const_int32__TestPropertyClass__TestInnerStruct_Property(BaseProperty *prop)
    {
        PairProperty *p = static_cast<PairProperty *>(prop);
        p->setFirstProperty(IReflectionRuntimeModule::getType<const int32>());
        p->setSecondProperty(IReflectionRuntimeModule::getType<TestPropertyClass::TestInnerStruct>());
        p->constructDataRetriever<PairDataRetrieverImpl<const int32, TestPropertyClass::TestInnerStruct>>();
    }

    static BaseProperty *createstd__set_uint64_Property()
    {
        BaseProperty *prop = new ContainerPropertyImpl<std::set<uint64>>(
            STRID("std::set<uint64>"), TCHAR("std::set<uint64>"), typeInfoFrom<std::set<uint64>>());
        return prop;
    }
    static void initstd__set_uint64_Property(BaseProperty *prop)
    {
        ContainerPropertyImpl<std::set<uint64>> *p = static_cast<ContainerPropertyImpl<std::set<uint64>> *>(prop);
        p->setElementProperty(IReflectionRuntimeModule::getType<std::set<uint64>>());
        p->constructDataRetriever<ContainerRetrieverImpl<std::set<uint64>>>();
    }

    static BaseProperty *createstd__map_int32__TestPropertyClass__TestInnerStruct_Property()
    {
        BaseProperty *prop = new MapProperty(
            STRID("std::map<int32, TestPropertyClass::TestInnerStruct>"), TCHAR("std::map<int32, TestPropertyClass::TestInnerStruct>"), typeInfoFrom<std::map<int32, TestPropertyClass::TestInnerStruct>>());
        return prop;
    }
    static void initstd__map_int32__TestPropertyClass__TestInnerStruct_Property(BaseProperty *prop)
    {
        MapProperty *p = static_cast<MapProperty *>(prop);
        p->setElementProperty(IReflectionRuntimeModule::getType<std::pair<const int32, TestPropertyClass::TestInnerStruct>>());
        p->setKeyValueProperties(
            IReflectionRuntimeModule::getType<int32>(), IReflectionRuntimeModule::getType<TestPropertyClass::TestInnerStruct>()
        );
        p->constructDataRetriever<MapDataRetrieverImpl<std::map<int32, TestPropertyClass::TestInnerStruct>>>();
    }

    static TestPropertyClass *TestPropertyClassCtor(String newName) { return new TestPropertyClass(newName); }
    static ClassProperty *createTestPropertyClassProperty()
    {
        ClassProperty *prop = (new ClassProperty(STRID("TestPropertyClass"), TCHAR("TestPropertyClass"), typeInfoFrom<TestPropertyClass>()));
        return prop;
    }
    static void initTestPropertyClassProperty(ClassProperty *prop)
    {
        prop->addCtorPtr()
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setFunctionReturnProperty(IReflectionRuntimeModule::getType<TestPropertyClass *>())
            ->addFunctionParamProperty(
                STRID("newName"), TCHAR("newName"), IReflectionRuntimeModule::getType<String>())->constructFuncPointer<GlobalFunctionWrapperImpl<TestPropertyClass *, String>>(&ThisType::TestPropertyClassCtor);
        prop->addMemberFunc(STRID("printNewNameStr"), TCHAR("printNewNameStr"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setFunctionReturnProperty(IReflectionRuntimeModule::getType<void>())
            ->constructFuncPointer<MemberFunctionWrapperImpl<const TestPropertyClass, void>>(&TestPropertyClass::printNewNameStr);

        prop->addStaticField(STRID("staticInteger"), TCHAR("staticInteger"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setField(IReflectionRuntimeModule::getType<int32>())
            ->constructFieldPtr<GlobalFieldWrapperImpl<int32>>(&TestPropertyClass::staticInteger);

        prop->addMemberField(STRID("idToSection"), TCHAR("idToSection"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setField(IReflectionRuntimeModule::getType<std::map<int32, TestPropertyClass::TestInnerStruct>>())
            ->constructFieldPtr<MemberFieldWrapperImpl<TestPropertyClass, std::map<int32, TestPropertyClass::TestInnerStruct>>>(
                &TestPropertyClass::idToSection
            );

        prop->addMemberField(STRID("newNameStr"), TCHAR("newNameStr"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setField(IReflectionRuntimeModule::getType<String>())
            ->constructFieldPtr<MemberFieldWrapperImpl<TestPropertyClass, String>>(&TestPropertyClass::newNameStr);

        prop->addMemberField(STRID("nextClass"), TCHAR("nextClass"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setField(IReflectionRuntimeModule::getType<TestPropertyClass *>())
            ->constructFieldPtr<MemberFieldWrapperImpl<TestPropertyClass, TestPropertyClass *>>(&TestPropertyClass::nextClass);

        prop->addMemberField(STRID("handles"), TCHAR("handles"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setField(IReflectionRuntimeModule::getType<std::set<uint64>>())
            ->constructFieldPtr<MemberFieldWrapperImpl<TestPropertyClass, std::set<uint64>>>(&TestPropertyClass::handles);
    }
};

struct RegisterPropertyFactory_TestPropertyClass_TestInnerStruct
{
    using ThisType = RegisterPropertyFactory_TestPropertyClass_TestInnerStruct;
    RegisterPropertyFactory_TestPropertyClass_TestInnerStruct()
    {
        IReflectionRuntimeModule::get()->registerTypeFactory(
            typeInfoFrom<std::vector<String>>(), { &ThisType::createstd_vector_StringProperty, &ThisType::initstd_vector_StringProperty }
        );

        IReflectionRuntimeModule::get()->registerClassFactory(
            STRID("TestPropertyClass::TestInnerStruct"), typeInfoFrom<TestPropertyClass::TestInnerStruct>(), { &ThisType::createTestPropertyClass_TestInnerStructProperty, &ThisType::initTestPropertyClass_TestInnerStructProperty }
        );
    }

    static BaseProperty *createstd_vector_StringProperty()
    {
        BaseProperty *prop = new ContainerPropertyImpl<std::vector<String>>(
            STRID("std::vector<String>"), TCHAR("std::vector<String>"), typeInfoFrom<std::vector<String>>());
        return prop;
    }
    static void initstd_vector_StringProperty(BaseProperty *prop)
    {
        ContainerPropertyImpl<std::vector<String>> *p = static_cast<ContainerPropertyImpl<std::vector<String>> *>(prop);
        p->setElementProperty(IReflectionRuntimeModule::getType<String>());
        p->constructDataRetriever<ContainerRetrieverImpl<std::vector<String>>>();
    }

    static ClassProperty *createTestPropertyClass_TestInnerStructProperty()
    {
        ClassProperty *prop = (new ClassProperty(
            STRID("TestPropertyClass::TestInnerStruct"), TCHAR("TestPropertyClass::TestInnerStruct"), typeInfoFrom<TestPropertyClass::TestInnerStruct>())
        );
        return prop;
    }
    static void initTestPropertyClass_TestInnerStructProperty(ClassProperty *prop)
    {
        prop->addMemberField(STRID("names"), TCHAR("names"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setField(IReflectionRuntimeModule::getType<std::vector<String>>())
            ->constructFieldPtr<MemberFieldWrapperImpl<TestPropertyClass::TestInnerStruct, std::vector<String>>>(
                &TestPropertyClass::TestInnerStruct::names
            );

        prop->addMemberField(STRID("numNames"), TCHAR("numNames"))
            ->setFieldAccessor(EPropertyAccessSpecifier::Public)
            ->setField(IReflectionRuntimeModule::getType<uint32>())
            ->constructFieldPtr<MemberFieldWrapperImpl<TestPropertyClass::TestInnerStruct, uint32>>(
                &TestPropertyClass::TestInnerStruct::numNames
            );
    }
};
void testPropertySystem()
{
    RegisterPropertyFactory_TestPropertyClass __zzz__RegisterPropertyFactory_TestPropertyClass;
    RegisterPropertyFactory_TestPropertyClass_TestInnerStruct __zzz__RegisterPropertyFactory_TestPropertyClass_TestInnerStruct;

    const ClassProperty *prop = IReflectionRuntimeModule::getClassType<TestPropertyClass>();
    for (const FunctionProperty *ctor : prop->constructors)
    {
        String args;
        if (!ctor->funcParamsProp.empty())
        {
            args += String(ctor->funcParamsProp[0].typeProperty->nameString) + TCHAR(" ") + ctor->funcParamsProp[0].nameString;
            for (int32 i = 1; i < ctor->funcParamsProp.size(); ++i)
            {
                args
                    += String(TCHAR(", ")) + ctor->funcParamsProp[i].typeProperty->nameString + TCHAR(" ") + ctor->funcParamsProp[i].nameString;
            }
        }
        LOG("Test", "Class %s: CTor %s(%s)", prop->name, ctor->name, args);
    }
    for (const FunctionProperty *memFunc : prop->memberFunctions)
    {
        String args;
        if (!memFunc->funcParamsProp.empty())
        {
            args += String(memFunc->funcParamsProp[0].typeProperty->nameString) + TCHAR(" ") + memFunc->funcParamsProp[0].nameString;
            for (int32 i = 1; i < memFunc->funcParamsProp.size(); ++i)
            {
                args += String(
                    TCHAR(", ")
                    )
                    + memFunc->funcParamsProp[i].typeProperty->nameString + TCHAR(" ") + memFunc->funcParamsProp[i].nameString;
            }
        }
        LOG("Test", "Class %s: Func %s %s(%s)", prop->nameString, memFunc->funcReturnProp->nameString, memFunc->nameString, args);
    }
    for (const FieldProperty *memField : prop->memberFields)
    {
        LOG("Test", "Class %s: Field %s %s;", prop->nameString, memField->field->nameString, memField->nameString);
    }
    TestPropertyClass *object = nullptr;
    if (static_cast<const GlobalFunctionWrapper *>(prop->constructors[0]->funcPtr)
            ->invoke<TestPropertyClass *, String>(object, TCHAR("Jeslas Pravin")))
    {
        static_cast<const MemberFunctionWrapper *>(prop->memberFunctions[0]->funcPtr)->invokeVoid(object);
    }
}

void testTemplateReflectionGeneration()
{
    String appName;
    String appDir = Paths::applicationDirectory(appName);
    std::vector<String> templateFiles = FileSystemFunctions::listFiles(
        PathFunctions::toAbsolutePath(TCHAR("../../../Source/Tools/ModuleReflectTool/Templates"), appDir), true, TCHAR("*.mustache") );
    std::unordered_map<String, MustacheStringFormatter> templates;
    templates.reserve(templateFiles.size());
    for (const String &filePath : templateFiles)
    {
        String fileContent;
        if (FileHelper::readString(fileContent, filePath) && !fileContent.empty())
        {
            templates.insert({ PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(filePath)),
                               MustacheStringFormatter(fileContent) });
        }
    }

    MustacheContext headerFileContext;
    headerFileContext.args[TCHAR("HeaderFileId")] = PropertyHelper::getValidSymbolName(appName);
    // #ReflectTypes contexts
    {
        std::vector<MustacheContext> reflectTypesContexts;

        MustacheContext &reflectClassCntx = reflectTypesContexts.emplace_back();
        reflectClassCntx.args[TCHAR("LineNumber")] = 10;
        reflectClassCntx.args[TCHAR("TypeName")] = TCHAR("TestPropertyClass");
        reflectClassCntx.args[TCHAR("IsClass")] = true;
        reflectClassCntx.args[TCHAR("IsBaseType")] = true;

        MustacheContext &reflectStructCntx = reflectTypesContexts.emplace_back();
        reflectStructCntx.args[TCHAR("LineNumber")] = 14;
        reflectStructCntx.args[TCHAR("TypeName")] = TCHAR("TestPropertyClass::TestInnerStruct");
        reflectStructCntx.args[TCHAR("IsClass")] = false;

        headerFileContext.sectionContexts.insert({ TCHAR("ReflectTypes"), std::move(reflectTypesContexts) });
    }

    MustacheContext sourceFileContext;
    sourceFileContext.args[TCHAR("HeaderFileId")] = PropertyHelper::getValidSymbolName(appName);
    sourceFileContext.args[TCHAR("HeaderInclude")] = PropertyHelper::getValidSymbolName(appName);

    std::vector<MustacheContext> allReflectTypes;
    // QualifiedTypes
    {
        MustacheContext &classPtr = allReflectTypes.emplace_back();
        classPtr.args[TCHAR("TypeName")] = TCHAR("TestPropertyClass*");
        classPtr.args[TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("TestPropertyClass *"));
        classPtr.args[TCHAR("PropertyTypeName")] = TCHAR("BaseProperty");
        classPtr.args[TCHAR("RegisterFunctionName")] = TCHAR("registerTypeFactory");

        sourceFileContext.sectionContexts[TCHAR("QualifiedTypes")].emplace_back(classPtr);
    }
    // PairTypes
    {
        MustacheContext &mapElemPair = allReflectTypes.emplace_back();
        mapElemPair.args[TCHAR("TypeName")] = TCHAR("std::pair<const int32, TestPropertyClass::TestInnerStruct>");
        mapElemPair.args
            [TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("std::pair<const int32, TestPropertyClass::TestInnerStruct>"));
        mapElemPair.args[TCHAR("PropertyTypeName")] = TCHAR("BaseProperty");
        mapElemPair.args[TCHAR("RegisterFunctionName")] = TCHAR("registerTypeFactory");

        sourceFileContext.sectionContexts[TCHAR("PairTypes")].emplace_back(mapElemPair);
    }
    // ContainerTypes
    {
        MustacheContext &setInt = allReflectTypes.emplace_back();
        setInt.args[TCHAR("TypeName")] = TCHAR("std::set<uint64>");
        setInt.args[TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("std::set<uint64>"));
        setInt.args[TCHAR("PropertyTypeName")] = TCHAR("BaseProperty");
        setInt.args[TCHAR("RegisterFunctionName")] = TCHAR("registerTypeFactory");
        sourceFileContext.sectionContexts[TCHAR("ContainerTypes")].emplace_back(setInt);

        MustacheContext &vectorStr = allReflectTypes.emplace_back();
        vectorStr.args[TCHAR("TypeName")] = TCHAR("std::vector<String>");
        vectorStr.args[TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("std::vector<String>"));
        vectorStr.args[TCHAR("PropertyTypeName")] = TCHAR("BaseProperty");
        vectorStr.args[TCHAR("RegisterFunctionName")] = TCHAR("registerTypeFactory");
        sourceFileContext.sectionContexts[TCHAR("ContainerTypes")].emplace_back(vectorStr);
    }
    // MapTypes
    {
        MustacheContext &mapType = allReflectTypes.emplace_back();
        mapType.args[TCHAR("TypeName")] = TCHAR("std::map<int32, TestPropertyClass::TestInnerStruct>");
        mapType.args[TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("std::map<int32, TestPropertyClass::TestInnerStruct>"));
        mapType.args[TCHAR("PropertyTypeName")] = TCHAR("BaseProperty");
        mapType.args[TCHAR("RegisterFunctionName")] = TCHAR("registerTypeFactory");

        sourceFileContext.sectionContexts[TCHAR("MapTypes")].emplace_back(mapType);
    }
    // EnumTypes
    {
        MustacheContext &enumType = allReflectTypes.emplace_back();
        enumType.args[TCHAR("TypeName")] = TCHAR("TestPropertyClass::ETestEnumType");
        enumType.args[TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("TestPropertyClass::ETestEnumType"));
        enumType.args[TCHAR("PropertyTypeName")] = TCHAR("EnumProperty");
        enumType.args[TCHAR("RegisterFunctionName")] = TCHAR("registerEnumFactory");

        MustacheContext &enumTypesContext = sourceFileContext.sectionContexts[TCHAR("EnumTypes")].emplace_back(enumType);
        enumTypesContext.args[TCHAR("CanUseAsFlags")] = false;
        enumTypesContext.args[TCHAR("TypeMetaFlags")] = 0;
        enumTypesContext.args[TCHAR("TypeMetaData")] = TCHAR("");
        std::vector<MustacheContext> &enumFields = enumTypesContext.sectionContexts[TCHAR("EnumFields")];
        {
            MustacheContext cntxt;
            cntxt.args[TCHAR("EnumFieldName")] = TCHAR("E1");
            cntxt.args[TCHAR("EnumFieldValue")] = uint64(TestPropertyClass::ETestEnumType::E1);
            cntxt.args[TCHAR("EnumFieldMetaFlags")] = 0;
            cntxt.args[TCHAR("EnumFieldMetaData")] = TCHAR("");
            enumFields.emplace_back(cntxt);

            cntxt = {};
            cntxt.args[TCHAR("EnumFieldName")] = TCHAR("E2");
            cntxt.args[TCHAR("EnumFieldValue")] = uint64(TestPropertyClass::ETestEnumType::E2);
            cntxt.args[TCHAR("EnumFieldMetaFlags")] = 0;
            cntxt.args[TCHAR("EnumFieldMetaData")] = TCHAR("");
            enumFields.emplace_back(cntxt);

            cntxt = {};
            cntxt.args[TCHAR("EnumFieldName")] = TCHAR("E3");
            cntxt.args[TCHAR("EnumFieldValue")] = uint64(TestPropertyClass::ETestEnumType::E3);
            cntxt.args[TCHAR("EnumFieldMetaFlags")] = 0;
            cntxt.args[TCHAR("EnumFieldMetaData")] = TCHAR("");
            enumFields.emplace_back(cntxt);
            cntxt = {};
            cntxt.args[TCHAR("EnumFieldName")] = TCHAR("E4");
            cntxt.args[TCHAR("EnumFieldValue")] = uint64(TestPropertyClass::ETestEnumType::E4);
            cntxt.args[TCHAR("EnumFieldMetaFlags")] = 0;
            cntxt.args[TCHAR("EnumFieldMetaData")] = TCHAR("");
            enumFields.emplace_back(cntxt);
        }
    }
    // ClassTypes
    {
        MustacheContext &classType = allReflectTypes.emplace_back();
        classType.args[TCHAR("TypeName")] = TCHAR("TestPropertyClass");
        classType.args[TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("TestPropertyClass"));
        classType.args[TCHAR("PropertyTypeName")] = TCHAR("ClassProperty");
        classType.args[TCHAR("RegisterFunctionName")] = TCHAR("registerClassFactory");

        MustacheContext &classTypeContext = sourceFileContext.sectionContexts[TCHAR("Classes")].emplace_back(classType);
        classTypeContext.args[TCHAR("TypeMetaFlags")] = 0;
        classTypeContext.args[TCHAR("TypeMetaData")] = TCHAR("");
        {
            std::vector<MustacheContext> &classCtors = classTypeContext.sectionContexts[TCHAR("Ctors")];
            {
                MustacheContext &ctor = classCtors.emplace_back();
                ctor.args[TCHAR("ParamsList")] = TCHAR("String");
                ctor.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                ctor.args[TCHAR("CtorMetaFlags")] = 0;
                ctor.args[TCHAR("CtorMetaData")] = TCHAR("");
                std::vector<MustacheContext> &ctorParamsCnxt = ctor.sectionContexts[TCHAR("ParamsListContext")];
                MustacheContext &ctorStrParam = ctorParamsCnxt.emplace_back();
                ctorStrParam.args[TCHAR("ParamName")] = TCHAR("newName");
                ctorStrParam.args[TCHAR("ParamTypeName")] = TCHAR("String");
            }
            std::vector<MustacheContext> &memFuncs = classTypeContext.sectionContexts[TCHAR("MemberFuncs")];
            {
                MustacheContext &memFunc = memFuncs.emplace_back();
                memFunc.args[TCHAR("FunctionName")] = TCHAR("printNewNameStr");
                memFunc.args[TCHAR("ReturnTypeName")] = TCHAR("void");
                memFunc.args[TCHAR("ParamsList")] = TCHAR("");
                memFunc.args[TCHAR("FuncConst")] = true;
                memFunc.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                memFunc.args[TCHAR("FuncMetaFlags")] = 0;
                memFunc.args[TCHAR("FuncMetaData")] = TCHAR("");

                MustacheContext &memFuncSetter = memFuncs.emplace_back();
                memFuncSetter.args[TCHAR("FunctionName")] = TCHAR("setNewNameStr");
                memFuncSetter.args[TCHAR("ReturnTypeName")] = TCHAR("void");
                memFuncSetter.args[TCHAR("ParamsList")] = TCHAR("String");
                memFuncSetter.args[TCHAR("FuncConst")] = false;
                memFuncSetter.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                memFuncSetter.args[TCHAR("FuncMetaFlags")] = 0;
                memFuncSetter.args[TCHAR("FuncMetaData")] = TCHAR("");
                std::vector<MustacheContext> &memFuncSetterParamsCnxt = memFuncSetter.sectionContexts[TCHAR("ParamsListContext")];
                MustacheContext &strParam = memFuncSetterParamsCnxt.emplace_back();
                strParam.args[TCHAR("ParamName")] = TCHAR("newName");
                strParam.args[TCHAR("ParamTypeName")] = TCHAR("const String &");
            }
            std::vector<MustacheContext> &staticFuncs = classTypeContext.sectionContexts[TCHAR("StaticFuncs")];
            {
                MustacheContext &staticFunc = staticFuncs.emplace_back();
                staticFunc.args[TCHAR("FunctionName")] = TCHAR("incAndPrintInt");
                staticFunc.args[TCHAR("ReturnTypeName")] = TCHAR("void");
                staticFunc.args[TCHAR("ParamsList")] = TCHAR("");
                staticFunc.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                staticFunc.args[TCHAR("FuncMetaFlags")] = 0;
                staticFunc.args[TCHAR("FuncMetaData")] = TCHAR("");
            }
            std::vector<MustacheContext> &memFields = classTypeContext.sectionContexts[TCHAR("MemberFields")];
            {
                MustacheContext &idToSectField = memFields.emplace_back();
                idToSectField.args[TCHAR("FieldName")] = TCHAR("idToSection");
                idToSectField.args[TCHAR("FieldTypeName")] = TCHAR("std::map<int32, TestPropertyClass::TestInnerStruct>");
                idToSectField.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                idToSectField.args[TCHAR("FieldMetaFlags")] = 0;
                idToSectField.args[TCHAR("FieldMetaData")] = TCHAR("");

                MustacheContext &strField = memFields.emplace_back();
                strField.args[TCHAR("FieldName")] = TCHAR("newNameStr");
                strField.args[TCHAR("FieldTypeName")] = TCHAR("String");
                strField.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                strField.args[TCHAR("FieldMetaFlags")] = 0;
                strField.args[TCHAR("FieldMetaData")] = TCHAR("");

                MustacheContext &nxtClsField = memFields.emplace_back();
                nxtClsField.args[TCHAR("FieldName")] = TCHAR("nextClass");
                nxtClsField.args[TCHAR("FieldTypeName")] = TCHAR("TestPropertyClass*");
                nxtClsField.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                nxtClsField.args[TCHAR("FieldMetaFlags")] = 0;
                nxtClsField.args[TCHAR("FieldMetaData")] = TCHAR("");

                MustacheContext &hndsField = memFields.emplace_back();
                hndsField.args[TCHAR("FieldName")] = TCHAR("handles");
                hndsField.args[TCHAR("FieldTypeName")] = TCHAR("std::set<uint64>");
                hndsField.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                hndsField.args[TCHAR("FieldMetaFlags")] = 0;
                hndsField.args[TCHAR("FieldMetaData")] = TCHAR("");
            }
            static int32 staticInteger;
            std::vector<MustacheContext> &staticFields = classTypeContext.sectionContexts[TCHAR("StaticFields")];
            {
                MustacheContext &staticIntField = staticFields.emplace_back();
                staticIntField.args[TCHAR("FieldName")] = TCHAR("staticInteger");
                staticIntField.args[TCHAR("FieldTypeName")] = TCHAR("int32");
                staticIntField.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                staticIntField.args[TCHAR("FieldMetaFlags")] = 0;
                staticIntField.args[TCHAR("FieldMetaData")] = TCHAR("");
            }
        }

        MustacheContext &structType = allReflectTypes.emplace_back();
        structType.args[TCHAR("TypeName")] = TCHAR("TestPropertyClass::TestInnerStruct");
        structType.args[TCHAR("SanitizedName")] = PropertyHelper::getValidSymbolName(TCHAR("TestPropertyClass::TestInnerStruct"));
        structType.args[TCHAR("PropertyTypeName")] = TCHAR("ClassProperty");
        structType.args[TCHAR("RegisterFunctionName")] = TCHAR("registerStructFactory");

        MustacheContext &structTypeContext = sourceFileContext.sectionContexts[TCHAR("Classes")].emplace_back(structType);
        structTypeContext.args[TCHAR("TypeMetaFlags")] = 0;
        structTypeContext.args[TCHAR("TypeMetaData")] = TCHAR("");
        {
            std::vector<MustacheContext> &memFields = structTypeContext.sectionContexts[TCHAR("MemberFields")];
            {
                MustacheContext &idToSectField = memFields.emplace_back();
                idToSectField.args[TCHAR("FieldName")] = TCHAR("names");
                idToSectField.args[TCHAR("FieldTypeName")] = TCHAR("std::vector<String>");
                idToSectField.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                idToSectField.args[TCHAR("FieldMetaFlags")] = 0;
                idToSectField.args[TCHAR("FieldMetaData")] = TCHAR("");

                MustacheContext &strField = memFields.emplace_back();
                strField.args[TCHAR("FieldName")] = TCHAR("numNames");
                strField.args[TCHAR("FieldTypeName")] = TCHAR("uint32");
                strField.args[TCHAR("AccessSpecifier")] = TCHAR("Public");
                strField.args[TCHAR("FieldMetaFlags")] = 0;
                strField.args[TCHAR("FieldMetaData")] = TCHAR("");
            }
        }
    }
    sourceFileContext.sectionContexts[TCHAR("AllRegisterTypes")] = allReflectTypes;

    // Write header file
    String headerContent = templates[TCHAR("ReflectedHeader")].render(headerFileContext, templates);
    PlatformFile headerFile(PathFunctions::combinePath(appDir, TCHAR("Saved"), TCHAR("Test"), appName + TCHAR(".gen.h")));
    headerFile.setCreationAction(EFileFlags::CreateAlways);
    headerFile.setFileFlags(EFileFlags::Write);
    headerFile.setSharingMode(EFileSharing::ReadOnly);
    headerFile.openOrCreate();
    headerFile.write({ reinterpret_cast<uint8 *>(headerContent.data()), uint32(headerContent.size()) });
    headerFile.closeFile();

    // Write source file
    String sourceContent = templates[TCHAR("ReflectedSource")].render(sourceFileContext, templates);
    PlatformFile srcFile(PathFunctions::combinePath(appDir, TCHAR("Saved"), TCHAR("Test"), appName + TCHAR(".gen.cpp")));
    srcFile.setCreationAction(EFileFlags::CreateAlways);
    srcFile.setFileFlags(EFileFlags::Write);
    srcFile.setSharingMode(EFileSharing::ReadOnly);
    srcFile.openOrCreate();
    srcFile.write({ reinterpret_cast<uint8 *>(sourceContent.data()), uint32(sourceContent.size()) });
    srcFile.closeFile();
}
} // namespace SampleCode