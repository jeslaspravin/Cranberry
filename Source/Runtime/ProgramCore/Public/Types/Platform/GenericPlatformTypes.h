/*!
 * \file GenericPlatformTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "String/String.h"

struct LibraryData
{
    String name;
    String imgPath;
    void *basePtr;
    dword moduleSize;
};

class GenericPlatformTypes
{
private:
    GenericPlatformTypes() = default;

public:
    using PlatformHandle = void *;

    using InstanceHandle = PlatformHandle;
    using LibHandle = PlatformHandle;
    using WindowHandle = PlatformHandle;

    using ProcAddress = void *;
};