/*!
 * \file ClangWrappers.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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

    ~CXStringWrapper() { clang_disposeString(str); }

    String toString() const
    {
        if (const AChar *cPtr = clang_getCString(str))
        {
            return UTF8_TO_TCHAR(cPtr);
        }
        return {};
    }
};

// Hashing overrides
FORCE_INLINE bool operator== (const CXCursor &lhs, const CXCursor &rhs) { return clang_equalCursors(lhs, rhs); }
template <>
struct std::hash<CXCursor>
{
    NODISCARD SizeT operator() (const CXCursor &keyval) const noexcept { return clang_hashCursor(keyval); }
};

// Logger overrides
FORCE_INLINE OutputStream &operator<< (OutputStream &stream, const CXStringRef &str)
{
    stream << str->toString();
    return stream;
}

FORCE_INLINE OutputStream &operator<< (OutputStream &stream, const CXString &cxStr)
{
    if (const AChar *cPtr = clang_getCString(cxStr))
    {
        stream << UTF8_TO_TCHAR(cPtr);
        clang_disposeString(cxStr);
    }
    return stream;
}

FORCE_INLINE OutputStream &operator<< (OutputStream &stream, const CXSourceLocation &cxStrLoc)
{
    CXFile file;
    uint32 lineNum, colNum;
    clang_getFileLocation(cxStrLoc, &file, &lineNum, &colNum, nullptr);
    String srcfile(CXStringWrapper(clang_getFileName(file)).toString());
    stream << srcfile << TCHAR("(") << lineNum << TCHAR(",") << colNum << TCHAR("):");
    return stream;
}
