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

#include "ICoreObjectsModule.h"
#include "Types/CoreDefines.h"
#include "CoreObjectsDB.h"

class CoreObjectsModule final : public ICoreObjectsModule
{
private:
    CoreObjectsDB objsDb;
public:
    /* IModuleBase overrides */
    void init() override;
    void release() override;

    /* ICoreObjectsModule overrides */
    const CoreObjectsDB& getObjectsDB() const override;

    /* Overrides ends */

    FORCE_INLINE static CoreObjectsModule* get()
    {
        return static_cast<CoreObjectsModule*>(ICoreObjectsModule::get());
    }
    FORCE_INLINE static CoreObjectsDB& objectsDB()
    {
        return get()->objsDb;
    }
};