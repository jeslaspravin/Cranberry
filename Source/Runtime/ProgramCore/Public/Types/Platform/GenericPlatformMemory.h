/*!
 * \file GenericPlatformMemory.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"

class CBMemory;
class CBMemAlloc;

// Why not use platform functions class? Platform function class does not allow default generic impl, for memory though we need it
class PROGRAMCORE_EXPORT GenericPlatformMemory
{
private:
    GenericPlatformMemory() = default;
public:

    static CBMemAlloc* createMemAllocator();
};