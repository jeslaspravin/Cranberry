/*!
 * \file IModuleBase.h
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

// Module life cycle interface
class PROGRAMCORE_EXPORT IModuleBase
{
public:
    virtual ~IModuleBase() = default;

    virtual void init() = 0;
    virtual void release() = 0;
};

class PROGRAMCORE_EXPORT ModuleNoImpl : public IModuleBase
{
public:
    void init() final {}
    void release() final {}
};
