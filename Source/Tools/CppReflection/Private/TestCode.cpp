#include "TestCode.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Containers/ArrayView.h"

#include <iostream>

// Logger overrides
std::ostream& operator<<(std::ostream& stream, const CppReflectionParser::CXStringRef& str)
{
    stream << clang_getCString(str->str);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const CXString& cxStr)
{
    stream << clang_getCString(cxStr);
    clang_disposeString(cxStr);
    return stream;
}

namespace CppReflectionParser
{
    void printDiagnostics(CXDiagnostic diagnostic, uint32 formatOptions)
    {
        CXDiagnosticSet childDiags = clang_getChildDiagnostics(diagnostic);
        const uint32 childDiagsNum = clang_getNumDiagnosticsInSet(childDiags);

        CXStringRef diagnosticStr(new CXStringWrapper(clang_formatDiagnostic(diagnostic, formatOptions)));
        Logger::warn("Diagnostics", "%s", diagnosticStr);
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
            access = "public";
            break;
        case CX_CXXProtected:
            access = "protected";
            break;
        case CX_CXXPrivate:
            access = "private";
            break;
        case CX_CXXInvalidAccessSpecifier:
        default:
            access = "Invalid";
            break;
        }
        return access;
    }

    void printJustTypeInfo(CXType type)
    {
        CXType canonicalType = clang_getCanonicalType(type);
        CXStringRef typeName(new CXStringWrapper(clang_getTypeSpelling(type)));
        switch (canonicalType.kind)
        {
        case CXType_RValueReference:
            Logger::log("CppReflectionParser", "%s() : Type %s is a r-value", __func__, typeName);
            break;
        case CXType_LValueReference:
            Logger::log("CppReflectionParser", "%s() : Type %s is a l-value", __func__, typeName);
            break;
        case CXType_Pointer:
        {
            // Get cursor to declaration of pointer's type
            // Use declaration if only that type is not basic POD type, If POD then just inner type will be same and child visitor will find the referenced type
            CXType innerType = clang_getPointeeType(canonicalType);
            int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
            Logger::log("CppReflectionParser", "%s() : Type %s - Inner type is %s and is const? %s", __func__, typeName
                , clang_getTypeSpelling(innerType)
                , !!(bIsInnerTypeConst)
                ? "true" : "false"
            );
            break;
        }
        case CXType_ConstantArray:
            Logger::log("CppReflectionParser", "%s() : Type %s container element count %d", __func__, typeName, clang_getNumElements(type));
        case CXType_IncompleteArray:
        case CXType_DependentSizedArray:
        case CXType_Vector:
        case CXType_VariableArray:
        {
            // Use declaration if only that type is not basic POD type, If POD then just inner type will be same and child visitor will find the referenced type
            CXType innerType = clang_getPointeeType(canonicalType);
            int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
            Logger::log("CppReflectionParser", "%s() : Type %s - Inner type is %s and is const? %s", __func__, typeName
                , clang_getTypeSpelling(innerType)
                , !!(bIsInnerTypeConst)
                ? "true" : "false"
            );
            break;
        }
        default:
            break;
        }
    }

    void printVariableTypeInfo(CXCursor cursor, SourceParsedInfo& srcParsedInfo, CXType fieldType, CXType fieldCanonicalType)
    {
        CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(cursor)));

        // The type can be considered const if its container is const or the type itself is const
        int32 bIsOuterTypeConst = clang_isConstQualifiedType(fieldCanonicalType);
        Logger::log("CppReflectionParser", "%s() : Field %s - Is const? %s", __func__, fieldName
            , !!(bIsOuterTypeConst)
            ? "true" : "false"
        );

        // Inner type will be different in case of atomic type or pointer or array or vector or complex
        CXCursor innerTypeCursor = cursor;
        switch (fieldCanonicalType.kind)
        {
        case CXType_RValueReference:
            Logger::log("CppReflectionParser", "%s() : Field %s is a r-value", __func__, fieldName);
            break;
        case CXType_LValueReference:
            Logger::log("CppReflectionParser", "%s() : Field %s is a l-value", __func__, fieldName);
            break;
        case CXType_Pointer:
        {
            // Get cursor to declaration of pointer's type
            // Use declaration if only that type is not basic POD type, If POD then just inner type will be same and child visitor will find the referenced type
            CXType innerType = clang_getPointeeType(fieldCanonicalType);
            if (!!clang_isPODType(fieldCanonicalType))
            {
                int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
                Logger::log("CppReflectionParser", "%s() : Field %s - Inner type %s is const? %s", __func__, fieldName
                    , clang_getTypeSpelling(innerType)
                    , !!(bIsInnerTypeConst)
                    ? "true" : "false"
                );
            }
            else
            {
                innerTypeCursor = clang_getTypeDeclaration(innerType);
            }
            Logger::log("CppReflectionParser", "%s() : Field %s - pointer inner type is %s", __func__, fieldName, clang_getTypeSpelling(innerType));
            break;
        }
        case CXType_ConstantArray:
            Logger::log("CppReflectionParser", "%s() : Field %s - container element count %d", __func__, fieldName, clang_getNumElements(fieldType));
        case CXType_IncompleteArray:
        case CXType_DependentSizedArray:
        case CXType_Vector:
        case CXType_VariableArray:
        {
            // Use declaration if only that type is not basic POD type, If POD then just inner type will be same and child visitor will find the referenced type
            CXType innerType = clang_getElementType(fieldCanonicalType);
            if (!!clang_isPODType(fieldCanonicalType))
            {
                int32 bIsInnerTypeConst = clang_isConstQualifiedType(innerType);
                Logger::log("CppReflectionParser", "%s() : Field %s - Element type %s is const? %s", __func__, fieldName
                    , clang_getTypeSpelling(innerType)
                    , !!(bIsInnerTypeConst)
                    ? "true" : "false"
                );
            }
            else
            {
                innerTypeCursor = clang_getTypeDeclaration(innerType);
            }
            Logger::log("CppReflectionParser", "%s() : Field %s - container element type is %s", __func__, fieldName, clang_getTypeSpelling(innerType));
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
            Logger::log("CppReflectionParser", "%s() : Field %s - Inner type %s is const? %s", __func__, fieldName
                , clang_getTypeSpelling(innerCanonicalType)
                , !!(bIsInnerTypeConst)
                ? "true" : "false"
            );
        }

        clang_visitChildren(innerTypeCursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CXCursorKind cursorKind = clang_getCursorKind(c);
                CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
                switch (cursorKind)
                {
                case CXCursor_TypeRef:
                {
                    // Just make sure we are using type alias's underlying canonical type
                    // Cannot get canonical cursor here as POD will not be having any cursor, so use canonical type instead
                    // CXCursor innerMostDeclTypeCursor = clang_getCanonicalCursor(c);
                    CXType innerMostType = clang_getCanonicalType(clang_getCursorType(c));
                    Logger::log("CppReflectionParser", "printVariableTypeInfo() : Field's innermost canonical type is %s", clang_getTypeSpelling(innerMostType));
                    break;
                }
                case CXCursor_AnnotateAttr:
                    break;
                default:
                    CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo*)(clientData));
                    break;
                }
                return CXChildVisit_Continue;
            }
        , &srcParsedInfo);
    }

    void printFunctionSignature(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        // This same can be obtained from CXType of function cursor using clang_getArgType for arg type at an index in this function type
        // clang_getResultType to find return type of this function type
        // and clang_getNumArgTypes to find total number of non template arguments

        CXType funcRetType = clang_getCursorResultType(cursor);
        int32 paramsCount = clang_Cursor_getNumArguments(cursor);
        std::vector<CXCursor> paramsCursor(paramsCount);
        for (uint32 i = 0; i < paramsCount; ++i)
        {
            paramsCursor[i] = clang_Cursor_getArgument(cursor, i);
        }

        String functionPath = String::join(srcParsedInfo.namespaceList.begin(), srcParsedInfo.namespaceList.end(), "::");
        CXStringRef functionName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        String functionParams;
        // print return type's param 
        printJustTypeInfo(funcRetType);
        {
            Logger::log("CppReflectionParser", "%s() : Function %s Arguments info ---->", __func__, functionName);
            std::vector<String> paramStrs;
            int32 i = 0;
            for (CXCursor c : paramsCursor)
            {
                String& paramDeclStr = paramStrs.emplace_back();
                CXType paramType = clang_getCursorType(c);
                CXStringRef paramTypeName(new CXStringWrapper(clang_getTypeSpelling(paramType)));
                CXStringRef paramName(new CXStringWrapper(clang_getCursorSpelling(c)));

                Logger::log("CppReflectionParser", "%s() : Argument %d Name %s Type %s", __func__, i, paramName, paramTypeName);
                printJustTypeInfo(paramType);

                paramDeclStr = clang_getCString(paramTypeName->str) + String(" ") + clang_getCString(paramName->str);
                ++i;
            }

            functionParams = String::join(paramStrs.cbegin(), paramStrs.cend(), ", ");
        }

        Logger::log("CppReflectionParser", "%s() : Function %s Signature is %s %s::%s(%s)", __func__, functionName
            , clang_getTypeSpelling(funcRetType), functionPath, functionName, functionParams);
    }

    void visitTUCusor(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
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
            // #TODO(Jeslas) : Add visit union function if needed
            visitStructs(cursor, srcParsedInfo);
            return;
        case CXCursor_ClassDecl:
            visitClasses(cursor, srcParsedInfo);
            return;
        case CXCursor_EnumDecl:
            break;
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
        //case CXCursor_ObjCSynthesizeDecl:
        //case CXCursor_ObjCDynamicDecl:
        //    break;
        // Referring types or aliases
        case CXCursor_CXXAccessSpecifier:
            break;
        // Obj C functions
        //case CXCursor_ObjCSuperClassRef:
        //case CXCursor_ObjCProtocolRef:
        //case CXCursor_ObjCClassRef:
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
        //case CXCursor_ObjCAtTryStmt:
        //case CXCursor_ObjCAtCatchStmt:
        //case CXCursor_ObjCAtFinallyStmt:
        //case CXCursor_ObjCAtThrowStmt:
        //case CXCursor_ObjCAtSynchronizedStmt:
        //case CXCursor_ObjCAutoreleasePoolStmt:
        //case CXCursor_ObjCForCollectionStmt:
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
            Logger::log("CppReflectionParser", "%s() : Cursor '%s' of kind '%s'", __func__, cursorSpelling, cursorKindSpelling);
        }
    }

    void visitNameSpace(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        // Since we just need namespace's name string alone
        CXStringRef namespaceName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef displayName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
        Logger::log("CppReflectionParser", "%s() : Namespace %s starts - Display name %s", __func__, namespaceName, displayName);
        srcParsedInfo.namespaceList.push_back(clang_getCString(namespaceName->str));

        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo*)(clientData));
                return CXChildVisit_Continue;
            }
            , &srcParsedInfo);

        srcParsedInfo.namespaceList.pop_back();
        Logger::log("CppReflectionParser", "%s() : Namespace %s ends", __func__, namespaceName);
    }

    void visitMacroDefinition(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        // Get cursor location and TU to get token at this location
        CXSourceLocation cursorSrcLoc = clang_getCursorLocation(cursor);
        CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

        CXToken* token = clang_getToken(tu, cursorSrcLoc);
        CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, *token)));
        CXStringRef macroName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        Logger::log("CppReflectionParser", "%s() : Macro %s defined as %s", __func__, macroName, tokenStr);

        // #TODO(Jeslas) : Find how to get macro's value and arguments if the cursor is function like macro
        clang_disposeTokens(tu, token, 1);
    }

    void visitMacroExpansion(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        // Get cursor location and TU to get token at this location
        CXSourceLocation cursorSrcLoc = clang_getCursorLocation(cursor);
        CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

        CXToken* token = clang_getToken(tu, cursorSrcLoc);
        CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, *token)));
        CXStringRef macroName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        Logger::log("CppReflectionParser", "%s() : Macro %s expanded as %s", __func__, macroName, tokenStr);

        // #TODO(Jeslas) : Find how to get macro's expanded value and arguments passed in if the cursor is function like macro
        clang_disposeTokens(tu, token, 1);
    }

    void visitIncludes(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        // Gets include's resolved file. It will be null if not resolved
        CXFile includeFile = clang_getIncludedFile(cursor);
        // Include file text 
        CXStringRef inclsName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        if (includeFile != nullptr)
        {
            // Resolved the file in disk and gives back resolved file path, Empty if file does not exists anymore
            CXStringRef inclsFilePath(new CXStringWrapper(clang_File_tryGetRealPathName(includeFile)));
            if (std::strlen(clang_getCString(inclsFilePath->str)) == 0)
            {
                // Gives the cached resolved path and file name
                inclsFilePath = new CXStringWrapper(clang_getFileName(includeFile));
            }
            Logger::log("CppReflectionParser", "%s() : \"%s\" include file resolved from %s", __func__, inclsName, inclsFilePath);
        }
        else
        {
            srcParsedInfo.includes.emplace_back(clang_getCString(inclsName->str));
            Logger::error("CppReflectionParser", "%s() : \"%s\" include file could not be resolved", __func__, inclsName);
        }
    }

    void visitClasses(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        // Since class defines new namespace for declared variables
        CXStringRef className(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef classDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
        Logger::log("CppReflectionParser", "%s() : Class %s starts - Display name %s", __func__, className, classDispName);
        srcParsedInfo.namespaceList.push_back(clang_getCString(className->str));
        String currAccessSpecifier = srcParsedInfo.scopeAccessSpecifier;
        srcParsedInfo.scopeAccessSpecifier = "private";

        String classPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), "::");
        Logger::log("CppReflectionParser", "%s() : Class full path name %s", __func__, classPathName);

        int32 bIsAbstract = clang_CXXRecord_isAbstract(cursor);
        if (!!bIsAbstract)
        {
            Logger::log("CppReflectionParser", "%s() : Class %s is abstract", __func__, className);
        }

        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CppReflectionParser::visitClassMember(c, *(SourceParsedInfo*)(clientData));
                return CXChildVisit_Continue;
            }
        , &srcParsedInfo);

        srcParsedInfo.scopeAccessSpecifier = currAccessSpecifier;
        srcParsedInfo.namespaceList.pop_back();
        Logger::log("CppReflectionParser", "%s() : Class %s ends", __func__, className);
    }

    void visitClassMember(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
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

            CXToken* tokens;
            uint32 tokensCount;
            clang_tokenize(tu, accessSpecDeclRange, &tokens, &tokensCount);

            fatalAssert(tokensCount > 1, "%s() : Tokens must be atleast 2(Got %d) in case of access specifiers 'public' and (':' or 'class/struct name')", __func__, tokensCount);
            CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[0])));
            access = clang_getCString(tokenStr->str);
#endif
            // Since we need just string to log now cursor spelling provides base record(class/struct) name. If need to find cursor/type one approach is to use same technique used in FriendDecl visitor below
            // To check if base struct is virtual(To avoid multiple inheritance of base type)
            int32 bIsBaseVirtual = clang_isVirtualBase(cursor);
            // To check if base type is abstract we need cursor of that type declaration, We get base type by getting cursor's type
            int32 bIsBaseAbstract = clang_CXXRecord_isAbstract(clang_getTypeDeclaration(clang_getCursorType(cursor)));
            Logger::log("CppReflectionParser", "%s() : Inherited from %s(%s and %s) with %s access specifier", __func__, cursorName
                , (!!bIsBaseAbstract)
                ? "Abstract" : "Non-Abstract"
                , (!!bIsBaseVirtual)
                ? "Virtual" : "Non-Virtual"
                , access);
            break; 
        }
        case CXCursor_AnnotateAttr:
            // Cursor spelling contains content of annotation
            Logger::log("CppReflectionParser", "%s() : [Access : %s] Annotated as %s", __func__, srcParsedInfo.scopeAccessSpecifier, cursorName);
            break;
        case CXCursor_CXXAccessSpecifier:
        {
            String access;
#if 0 // Use switch to find access? Both case works
            access = accessSpecifierStr(cursor);
#else // Use source range to read access specifier directly, Both case works
            // AccessSpecDecl - Source range from access specifier token to colon ':' token
            CXSourceRange accessSpecDeclRange = clang_getCursorExtent(cursor);

            CXToken* tokens;
            uint32 tokensCount;
            clang_tokenize(tu, accessSpecDeclRange, &tokens, &tokensCount);

            for (uint32 i = 0; i < tokensCount; ++i)
            {
                CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[i])));
                const AChar* str = clang_getCString(tokenStr->str);
                if (strcmp(str, ":") != 0)
                {
                    access += str;
                }
            }
#endif
            Logger::log("CppReflectionParser", "%s() : Previous access %s new access is %s", __func__, srcParsedInfo.scopeAccessSpecifier, access);
            srcParsedInfo.scopeAccessSpecifier = access;
            break;
        }
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypedefDecl:
        {
            // clang_getCursorType(cursor) gives type def type while clang_getTypedefDeclUnderlyingType(cursor) gives actual type that is being aliased
            CXType type = clang_getTypedefDeclUnderlyingType(cursor);
            // Since this typedef/using decl might be type alias by itself get its canonical type
            type = clang_getCanonicalType(type);
            Logger::log("CppReflectionParser", "%s() : %s type is being aliased as %s", __func__, clang_getTypeSpelling(type), cursorName);
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

    void visitClassFriendDecl(CXCursor cursor, SourceParsedInfo& srcParsedInfo, CXTranslationUnit tu)
    {
        // Cursor spelling or display name do not provide any information about who this friend is
        // Has no cursor type kind
        // Source range of FriendDecl however gives the entire declaration, And it can be obtained using clang_getCursorExtent
        String friendDeclStr;
        CXStringRef friendedType;
        {
            // FriendDecl - Source range will be from friend token to token before ';'
            CXSourceRange friendDeclRange = clang_getCursorExtent(cursor);
            // To skip friend we get source location token as well and skip it alone, as getLocation() gives where friend keyword ends
            CXSourceLocation friendEndLoc = clang_getCursorLocation(cursor);

            CXToken* tokens;
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
                        // Below reference to canonical type is not necessary as getting cursor type from reference cursor still prints correct type name
                        // However in case of typedef or using it still prints only alias names, So If we need canonical type we need to use below deref type and then find actual canonical type or use `clang_getCanonicalType(cursorType)`
                        //                        
                        // This will always be just a reference find the referenced type
                        //friendTypeCursor = clang_getCursorReferenced(friendTypeCursor);
                        //// Once TypeRef is converted to TypeDecl find if it is TypeDefNamedDecl or TypeAliasDecl
                        //while (clang_getCursorKind(friendTypeCursor) == CXCursorKind::CXCursor_TypedefDecl 
                        //    || clang_getCursorKind(friendTypeCursor) == CXCursorKind::CXCursor_TypeAliasDecl)
                        //{
                        //    friendTypeCursor = clang_getTypeDeclaration(clang_getTypedefDeclUnderlyingType(friendTypeCursor));
                        //}
                    }

                    CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[i])));
                    tokensStr.emplace_back(clang_getCString(tokenStr->str));
                }
            }
            friendDeclStr = String::join(tokensStr.cbegin(), tokensStr.cend(), " ");

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
        Logger::log("CppReflectionParser", "%s() : [Access : %s] %s(%s) is a friend of class %s"
            , __func__, srcParsedInfo.scopeAccessSpecifier, friendedType, friendDeclStr, srcParsedInfo.namespaceList.back());
    }

    void visitStructs(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        // Since struct defines new namespace for declared variables
        CXStringRef structName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef structDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
        Logger::log("CppReflectionParser", "%s() : Struct %s starts - Display name %s", __func__, structName, structDispName);
        srcParsedInfo.namespaceList.push_back(clang_getCString(structName->str));
        String currAccessSpecifier = srcParsedInfo.scopeAccessSpecifier;
        srcParsedInfo.scopeAccessSpecifier = "public";

        String structPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), "::");
        Logger::log("CppReflectionParser", "%s() : Struct full path name %s", __func__, structPathName);

        // Since cursor is struct declaration
        int32 bIsAbstract = clang_CXXRecord_isAbstract(cursor);
        if (!!bIsAbstract)
        {
            Logger::log("CppReflectionParser", "%s() : Struct %s is abstract", __func__, structName);
        }

        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CppReflectionParser::visitStructMember(c, *(SourceParsedInfo*)(clientData));
                return CXChildVisit_Continue;
            }
        , &srcParsedInfo);

        srcParsedInfo.scopeAccessSpecifier = currAccessSpecifier;
        srcParsedInfo.namespaceList.pop_back();
        Logger::log("CppReflectionParser", "%s() : Struct %s ends", __func__, structName);
    }

    void visitStructMember(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
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

            CXToken* tokens;
            uint32 tokensCount;
            clang_tokenize(tu, accessSpecDeclRange, &tokens, &tokensCount);

            fatalAssert(tokensCount > 1, "%s() : Tokens must be atleast 2(Got %d) in case of access specifiers 'public' and (':' or 'class/struct name')", __func__, tokensCount);
            CXStringRef tokenStr(new CXStringWrapper(clang_getTokenSpelling(tu, tokens[0])));
            access = clang_getCString(tokenStr->str);
#endif
            // Since we need just string to log now cursor spelling provides base record(class/struct) name. If need to find cursor/type one approach is to use same technique used in FriendDecl visitor below
            // To check if base struct is virtual(To avoid multiple inheritance of base type)
            int32 bIsBaseVirtual = clang_isVirtualBase(cursor);
            // To check if base type is abstract we need cursor of that type declaration, We get base type by getting cursor's type
            int32 bIsBaseAbstract = clang_CXXRecord_isAbstract(clang_getTypeDeclaration(clang_getCursorType(cursor)));
            Logger::log("CppReflectionParser", "%s() : Inherited from %s(%s and %s) with %s access specifier", __func__, cursorName
                , (!!bIsBaseAbstract)
                ? "Abstract" : "Non-Abstract"
                , (!!bIsBaseVirtual)
                ? "Virtual" : "Non-Virtual"
                , access);
            break;
        }
        case CXCursor_AnnotateAttr:
            // Cursor spelling contains content of annotation
            Logger::log("CppReflectionParser", "%s() : [Access : %s] Annotated as %s", __func__, srcParsedInfo.scopeAccessSpecifier, cursorName);
            break;
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypedefDecl:
        {
            // clang_getCursorType(cursor) gives type def type while clang_getTypedefDeclUnderlyingType(cursor) gives actual type that is being aliased
            CXType type = clang_getTypedefDeclUnderlyingType(cursor);
            // Since this typedef/using decl might be type alias by itself get its canonical type
            type = clang_getCanonicalType(type);
            Logger::log("CppReflectionParser", "%s() : %s type is being aliased as %s", __func__, clang_getTypeSpelling(type), cursorName);
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

    void visitMemberField(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef fieldDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
        Logger::log("CppReflectionParser", "%s() : Field %s - Display name %s", __func__, fieldName, fieldDispName);

        String fieldPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), "::");
        Logger::log("CppReflectionParser", "%s() : Field %s - Base path name %s", __func__, fieldName, fieldPathName);

        CXType fieldType = clang_getCursorType(cursor);
        CXType fieldCanonicalType = clang_getCanonicalType(fieldType);
        CXStringRef typeKindName(new CXStringWrapper(clang_getTypeKindSpelling(fieldCanonicalType.kind)));
        CXStringRef canonicalTypeName(new CXStringWrapper(clang_getTypeSpelling(fieldCanonicalType)));
        CXStringRef typeName = canonicalTypeName;
        if (!clang_equalTypes(fieldType, fieldCanonicalType))
        {
            typeName = new CXStringWrapper(clang_getTypeSpelling(fieldType));
        }
        Logger::log("CppReflectionParser", "%s() : Field %s - Field typename %s, Canonical typename %s, Type kind %s", __func__, fieldName, typeName, canonicalTypeName, typeKindName);
        if (fieldType.kind == CXType_LValueReference)
        {
            Logger::error("CppReflectionParser", "%s() : Field %s - Having reference member field is not good!", __func__, fieldName);
            return;
        }

        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CXCursorKind cursorKind = clang_getCursorKind(c);
                CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
                CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(p)));
                switch (cursorKind)
                {
                case CXCursor_AnnotateAttr:
                    // Cursor spelling contains content of annotation
                    Logger::log("CppReflectionParser", "visitMemberField() : Field %s - Annotated as %s", fieldName, cursorName);
                    break;
                default:
                    break;
                }
                return CXChildVisit_Continue;
            }
        , & srcParsedInfo);

        printVariableTypeInfo(cursor, srcParsedInfo, fieldType, fieldCanonicalType);
    }

    void visitVariableDecl(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        CXStringRef varName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef varDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
        Logger::log("CppReflectionParser", "%s() : Variable %s - Display name %s", __func__, varName, varDispName);

        String fieldPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), "::");
        Logger::log("CppReflectionParser", "%s() : Variable %s - Base path name %s", __func__, varName, fieldPathName);

        CXType fieldType = clang_getCursorType(cursor);
        CXType fieldCanonicalType = clang_getCanonicalType(fieldType);
        CXStringRef typeKindName(new CXStringWrapper(clang_getTypeKindSpelling(fieldCanonicalType.kind)));
        CXStringRef canonicalTypeName(new CXStringWrapper(clang_getTypeSpelling(fieldCanonicalType)));
        CXStringRef typeName = canonicalTypeName;
        if (!clang_equalTypes(fieldType, fieldCanonicalType))
        {
            typeName = new CXStringWrapper(clang_getTypeSpelling(fieldType));
        }
        Logger::log("CppReflectionParser", "%s() : Variable %s - Variable typename %s, Canonical typename %s, Type kind %s", __func__, varName, typeName, canonicalTypeName, typeKindName);

        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CXCursorKind cursorKind = clang_getCursorKind(c);
                CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
                CXStringRef fieldName(new CXStringWrapper(clang_getCursorSpelling(p)));
                switch (cursorKind)
                {
                case CXCursor_AnnotateAttr:
                    // Cursor spelling contains content of annotation
                    Logger::log("CppReflectionParser", "visitVariableDecl() : Field %s - Annotated as %s", fieldName, cursorName);
                    break;
                default:
                    break;
                }
                return CXChildVisit_Continue;
            }
        , &srcParsedInfo);

        printVariableTypeInfo(cursor, srcParsedInfo, fieldType, fieldCanonicalType);
    }

    void visitNonMemberFunctions(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef funcDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
        Logger::log("CppReflectionParser", "%s() : Function %s - Display name %s", __func__, funcName, funcDispName);

        String funcPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), "::");
        Logger::log("CppReflectionParser", "%s() : Function %s - Base path name %s", __func__, funcName, funcPathName);

        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CXCursorKind cursorKind = clang_getCursorKind(c);
                CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
                CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(p)));
                switch (cursorKind)
                {
                case CXCursor_AnnotateAttr:
                    // Cursor spelling contains content of annotation
                    Logger::log("CppReflectionParser", "visitNonMemberFunctions() : Function %s - Annotated as %s", funcName, cursorName);
                    break;
                case CXCursor_ParmDecl:
                    // Since we handle this in print function signature
                    break;
                default:
                    CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo*)(clientData));
                    break;
                }
                return CXChildVisit_Continue;
            }
        , &srcParsedInfo);

        printFunctionSignature(cursor, srcParsedInfo);
    }

    void visitMemberCppMethods(CXCursor cursor, SourceParsedInfo& srcParsedInfo)
    {
        CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(cursor)));
        CXStringRef funcDispName(new CXStringWrapper(clang_getCursorDisplayName(cursor)));
        Logger::log("CppReflectionParser", "%s() : Function %s - Display name %s", __func__, funcName, funcDispName);

        String funcPathName = String::join(srcParsedInfo.namespaceList.cbegin(), srcParsedInfo.namespaceList.cend(), "::");
        Logger::log("CppReflectionParser", "%s() : Function %s - Base path name %s", __func__, funcName, funcPathName);

        int32 bIsPureVirtual = clang_CXXMethod_isPureVirtual(cursor);
        int32 bIsVirtual = clang_CXXMethod_isVirtual(cursor);
        int32 bIsStatic = clang_CXXMethod_isStatic(cursor);
        int32 bIsConst = clang_CXXMethod_isConst(cursor);
        Logger::log("CppReflectionParser", "%s() : Function %s - %s%s", __func__, funcName
            , (!!bIsStatic)
            ? "Static and "
            : (!!bIsConst)
            ? "Const and " : ""
            , (!!bIsVirtual)
            ? (!!bIsPureVirtual)
            ? "Pure virtual" : "virtual"
            : "Non-virtual"
        );
        // If not pure virtual method then print all base methods
        if(bIsVirtual && !bIsPureVirtual)
        {
            uint32 numOverrides;
            CXCursor* baseCursors;
            // Gives up-to only one level
            clang_getOverriddenCursors(cursor, &baseCursors, &numOverrides);

            uint32 levelFromThisOverride = 1;
            Logger::log("CppReflectionParser", "%s() : Function %s - Overrides following methods ---->", __func__, funcName);
            std::vector<ArrayView<CXCursor>> currOverridenCursors{ ArrayView<CXCursor>(baseCursors, numOverrides) };
            while (!currOverridenCursors.empty())
            {
                std::vector<ArrayView<CXCursor>> newOverridenCursors;
                for (ArrayView<CXCursor>& overridenCursors : currOverridenCursors)
                {
                    for (uint32 i = 0; i < overridenCursors.size(); ++i)
                    {
                        // Get the class that this overridden method's cursor belongs to by getting type of the method and getting the type's class type
                        // Below does not provider parent class as cursor is not member pointer type
                        // CXType overridenClassType = clang_Type_getClassType(clang_getCursorType(overridenCursors[i]));
                        // Semantic parent gives place where this cursor is declared in
                        CXType overridenClassType = clang_getCursorType(clang_getCursorSemanticParent(overridenCursors[i]));
                        Logger::log("CppReflectionParser", "%s() : Function %s - (Level %d) method %s of %s", __func__, funcName
                            , levelFromThisOverride, clang_getCursorSpelling(overridenCursors[i]), clang_getTypeSpelling(overridenClassType));

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

        // The caller's reference type(lvalue or rvalue) else if no valid values then pointer or lvalue that calls this overload of the function
        // If none `retType func(Params...)` or `retType func(Params...) const` are allowed overloads and corresponding function gets called
        // If lvalue `retType func(Params...) &` or `retType func(Params...) const &` gets called
        // If rvalue `retType func(Params...) &&` gets called
        CXRefQualifierKind methodCalledRefKind = clang_Type_getCXXRefQualifier(clang_getCursorType(cursor));
        if (methodCalledRefKind != CXRefQualifier_None)
        {
            Logger::log("CppReflectionParser", "%s() : Function %s can be called from %s-value reference only", __func__
                , funcName
                , (methodCalledRefKind == CXRefQualifier_LValue)
                ? "l" : "r"
            );
        }
        
        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor p, CXClientData clientData)
            {
                CXCursorKind cursorKind = clang_getCursorKind(c);
                CXStringRef cursorName(new CXStringWrapper(clang_getCursorSpelling(c)));
                CXStringRef funcName(new CXStringWrapper(clang_getCursorSpelling(p)));
                switch (cursorKind)
                {
                case CXCursor_CXXFinalAttr:
                    Logger::log("CppReflectionParser", "visitMemberCppMethods() : Function %s - virtual is made final", funcName);
                    break;
                case CXCursor_CXXOverrideAttr:
                    Logger::log("CppReflectionParser", "visitMemberCppMethods() : Function %s - Has attribute override", funcName);
                    break;
                case CXCursor_AnnotateAttr:
                    // Cursor spelling contains content of annotation
                    Logger::log("CppReflectionParser", "visitMemberCppMethods() : Function %s - Annotated as %s", funcName, cursorName);
                    break;
                case CXCursor_ParmDecl:
                    // Since we handle this in print function signature
                    break;
                default:
                    CppReflectionParser::visitTUCusor(c, *(SourceParsedInfo*)(clientData));
                    break;
                }
                return CXChildVisit_Continue;
            }
        , &srcParsedInfo);

        printFunctionSignature(cursor, srcParsedInfo);
    }

}

namespace TestCode
{
    void testCode(String srcDir) noexcept
    {
        CXIndex index = clang_createIndex(0, 0);
        String argRefParseDef("-D__REF_PARSE__");
        String argIncludeModulePublic("-ID:/Workspace/VisualStudio/GameEngine/Source/Runtime/ProgramCore/Public");
        String argIncludeModuleGen("-ID:/Workspace/VisualStudio/GameEngine/Build/Source/Runtime/ProgramCore/Generated/Public");
        const AChar* args[] = { argIncludeModuleGen.getChar(), argIncludeModulePublic.getChar(), argRefParseDef.getChar()};
        // Use parse TU functions if need to customize certain options while compiling
        CXTranslationUnit unit = clang_parseTranslationUnit(
            index,
            FileSystemFunctions::combinePath(srcDir, "Header.H").getChar()
            , args, 3, nullptr, 0, CXTranslationUnit_DetailedPreprocessingRecord);
        //CXTranslationUnit unit = clang_createTranslationUnitFromSourceFile(index
        //    , FileSystemFunctions::combinePath(srcDir, "Header.H").getChar()
        //    , 3, args, 0, nullptr);
        if (unit == nullptr)
        {
            Logger::error("TestCode", "Unable to parse translation unit. Quitting.");
            return;
        }
        else
        {
            uint32 formatOptions = CXDiagnostic_DisplaySourceLocation | CXDiagnostic_DisplayColumn | CXDiagnostic_DisplayCategoryName | CXDiagnostic_DisplayOption;
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
                    CppReflectionParser::visitTUCusor(c, *(CppReflectionParser::SourceParsedInfo*)(client_data));
                }
                // Continue to next Cursor in TU
                return CXChildVisit_Continue;
            },
            &parsedInfo);

        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(index);
    }
}