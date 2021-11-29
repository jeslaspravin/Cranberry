#include "TestCode.h"
#include "clang-c/Index.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformFunctions.h"

#include <iostream>

std::ostream& operator<<(std::ostream& stream, const CXString& str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

namespace TestCode
{

    void testCode(String srcDir) noexcept
    {
        CXIndex index = clang_createIndex(0, 0);
        CXTranslationUnit unit = clang_parseTranslationUnit(
            index,
            FileSystemFunctions::combinePath(srcDir, "Header.hpp").getChar(), nullptr, 0,
            nullptr, 0,
            CXTranslationUnit_None);
        if (unit == nullptr)
        {
            Logger::error("TestCode", "Unable to parse translation unit. Quitting.");
            return;
        }

        CXCursor cursor = clang_getTranslationUnitCursor(unit);
        clang_visitChildren(
            cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data)
            {
                Logger::log("TestCode", "Cursor '%s' of kind '%s'", clang_getCursorSpelling(c), clang_getCursorKindSpelling(clang_getCursorKind(c)));
                return CXChildVisit_Recurse;
            },
            nullptr);

        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(index);
    }

}