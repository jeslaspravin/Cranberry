/*!
 * \file CoreObjectsModule.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CoreObjectsModule.h"
#include "CoreObjectAllocator.h"
#include "Modules/ModuleManager.h"
#include "CBEObjectHelpers.h"
#include "CBEPackage.h"
#include "CoreObjectDelegates.h"

#define MODULE_NAME CoreObjects
DECLARE_MODULE(MODULE_NAME, CoreObjectsModule)

ICoreObjectsModule *ICoreObjectsModule::get()
{
    return ModuleManager::get()->getOrLoadModulePtr<ICoreObjectsModule>(TCHAR(MACRO_TO_STRING(MODULE_NAME)));
}
const CoreObjectsDB &ICoreObjectsModule::objectsDB() { return CoreObjectsModule::objectsDB(); }

CoreObjectsDB *CoreObjectsModule::objsDbPtr = nullptr;

void CoreObjectsModule::init()
{
    cbe::initializeObjectAllocators();
    objsDbPtr = &objsDb;

    CoreObjectDelegates::onContentDirectoryAdded.bindObject(&packMan, &CBEPackageManager::registerContentRoot);
    CoreObjectDelegates::onContentDirectoryRemoved.bindObject(&packMan, &CBEPackageManager::registerContentRoot);
    CoreObjectDelegates::onObjectDestroyed.bindObject(&packMan, &CBEPackageManager::onObjectDeleted);
}

void CoreObjectsModule::release()
{
    CoreObjectDelegates::onContentDirectoryAdded.unbindAll(&packMan);
    CoreObjectDelegates::onContentDirectoryRemoved.unbindAll(&packMan);
    CoreObjectDelegates::onObjectDestroyed.unbindAll(&packMan);

    objsDbPtr = nullptr;
}

cbe::Package *CoreObjectsModule::getTransientPackage() const { return cbe::getDefaultObject<cbe::Package>(); }

CoreObjectGC &CoreObjectsModule::getGC() { return gc; }

CoreObjectsModule *CoreObjectsModule::get() { return static_cast<CoreObjectsModule *>(ICoreObjectsModule::get()); }
