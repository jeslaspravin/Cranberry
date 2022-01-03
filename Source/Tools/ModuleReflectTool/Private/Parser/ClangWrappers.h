#pragma once

#include "Types/Containers/ReferenceCountPtr.h"

#include <clang-c/Index.h>

struct CXStringWrapper;

using CXStringRef = ReferenceCountPtr<CXStringWrapper>;
struct CXStringWrapper : public RefCountable
{
    CXString str;

    CXStringWrapper(CXString inStr)
        : str(inStr)
    {}

    ~CXStringWrapper()
    {
        clang_disposeString(str);
    }

    String toString() const
    {
        if (const AChar* cPtr = clang_getCString(str))
        {
            return cPtr;
        }
        return {};
    }
};

// Logger overrides
FORCE_INLINE std::ostream& operator<<(std::ostream& stream, const CXStringRef& str)
{
    stream << str->toString();
    return stream;
}

FORCE_INLINE std::ostream& operator<<(std::ostream& stream, const CXString& cxStr)
{
    if (const AChar* cPtr = clang_getCString(cxStr))
    {
        stream << cPtr;
        clang_disposeString(cxStr);
    }
    return stream;
}

FORCE_INLINE std::ostream& operator<<(std::ostream& stream, const CXSourceLocation& cxStrLoc)
{
    CXFile file;
    uint32 lineNum, colNum;
    clang_getFileLocation(cxStrLoc, &file, &lineNum, &colNum, nullptr);
    String srcfile(CXStringWrapper(clang_getFileName(file)).toString());
    stream << srcfile << "(" << lineNum << "," << colNum << "):";
    return stream;
}
