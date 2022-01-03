#pragma once

#include "String/String.h"
#include "Types/Containers/ReferenceCountPtr.h"

#include <clang-c/Index.h>

namespace CppReflectionParser
{
    struct SourceParsedInfo
    {
        bool bHasGenerateMacro;
        // Empty inside none access specifiable region, We do not need this as `clang_getCXXAccessSpecifier` gives exactly that for this cursor
        String scopeAccessSpecifier;
        std::vector<String> includes;
        std::vector<String> namespaceList;
    };


    void printDiagnostics(CXDiagnostic diagnostic, uint32 formatOptions);
    String accessSpecifierStr(CXCursor cursor);
    void printJustTypeInfo(CXType type);
    void printVariableTypeInfo(CXCursor cursor, SourceParsedInfo& srcParsedInfo, CXType fieldType, CXType fieldCanonicalType);
    void printFunctionSignature(CXCursor cursor, SourceParsedInfo& srcParsedInfo);

    void visitTUCusor(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    void visitNameSpace(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    void visitMacroDefinition(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    void visitMacroExpansion(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    void visitIncludes(CXCursor cursor, SourceParsedInfo& srcParsedInfo);

    void visitClasses(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    void visitClassMember(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    void visitClassFriendDecl(CXCursor cursor, SourceParsedInfo& srcParsedInfo, CXTranslationUnit tu);

    void visitStructs(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    void visitStructMember(CXCursor cursor, SourceParsedInfo& srcParsedInfo);

    void visitEnums(CXCursor cursor, SourceParsedInfo& srcParsedInfo);

    void visitMemberField(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    // For both static and global variable declarations and variables inside function definition
    void visitVariableDecl(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    // For member methods
    void visitMemberCppMethods(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
    // For any c style/non member functions
    void visitNonMemberFunctions(CXCursor cursor, SourceParsedInfo& srcParsedInfo);
}

namespace SampleCode
{
    void testLibClangParsing(String srcDir) noexcept;
    void testTypesAndProperties();
    void testPropertySystem();
    void testRegex();
    void testTemplateReflectionGeneration();
}