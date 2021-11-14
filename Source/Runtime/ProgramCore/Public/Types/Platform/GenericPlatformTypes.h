#pragma once

#include "String/String.h"
#include "ProgramCoreExports.h"

struct PROGRAMCORE_EXPORT LibPointer 
{

    virtual ~LibPointer() = default;

};

typedef LibPointer* LibPointerPtr;

struct LibraryData
{
    String name;
    String imgName;
    void* basePtr;
    dword moduleSize;
};
