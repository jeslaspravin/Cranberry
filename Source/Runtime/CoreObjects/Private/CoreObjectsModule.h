/*!
 * \file CoreObjectsModule.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoreObjectGC.h"
#include "CoreObjectsDB.h"
#include "Serialization/CBEPackageManager.h"
#include "ICoreObjectsModule.h"
#include "Types/CoreDefines.h"

class CoreObjectsModule final : public ICoreObjectsModule
{
private:
    CoreObjectsDB objsDb;
    CoreObjectGC gc;
    CBEPackageManager packMan;

public:
    /* IModuleBase overrides */
    void init() override;
    void release() override;

    /* ICoreObjectsModule overrides */
    const CoreObjectsDB &getObjectsDB() const override;
    cbe::Package *getTransientPackage() const override;
    CoreObjectGC &getGC() override;
    /* Overrides ends */

    static CoreObjectsModule *get();
    FORCE_INLINE static CoreObjectsDB &objectsDB() { return get()->objsDb; }
    FORCE_INLINE static CBEPackageManager &packageManager() { return get()->packMan; }
};