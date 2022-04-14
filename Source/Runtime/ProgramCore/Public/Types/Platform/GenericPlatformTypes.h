/*!
 * \file GenericPlatformTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "String/String.h"

// #TODO(Jeslas) : Replace this type of heap allocated handles to raw platform handle here, PlatformFile,
// Application Instance, Window Instance
struct PROGRAMCORE_EXPORT LibPointer
{

    virtual ~LibPointer() = default;
};

typedef LibPointer *LibPointerPtr;

struct LibraryData
{
    String name;
    String imgName;
    void *basePtr;
    dword moduleSize;
};
