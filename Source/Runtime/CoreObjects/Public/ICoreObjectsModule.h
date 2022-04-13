/*!
 * \file ICoreObjectsModule.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Modules/IModuleBase.h"
#include "CoreObjectsExports.h"

class CoreObjectsDB;
class CoreObjectGC;

class COREOBJECTS_EXPORT ICoreObjectsModule : public IModuleBase
{
public:
    virtual const CoreObjectsDB& getObjectsDB() const = 0;
    virtual CoreObjectGC& getGC() = 0;
	static ICoreObjectsModule* get();
};