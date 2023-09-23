/*!
 * \file ICoreObjectsModule.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoreObjectsExports.h"
#include "Modules/IModuleBase.h"

class CoreObjectsDB;
class CoreObjectGC;
namespace cbe
{
class Package;
}

class COREOBJECTS_EXPORT ICoreObjectsModule : public IModuleBase
{
public:
    virtual CoreObjectGC &getGC() = 0;
    virtual cbe::Package *getTransientPackage() const = 0;

    static ICoreObjectsModule *get();
    static const CoreObjectsDB &objectsDB();
};
