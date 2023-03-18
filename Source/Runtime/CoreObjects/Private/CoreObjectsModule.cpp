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

DECLARE_MODULE(CoreObjects, CoreObjectsModule)

ICoreObjectsModule *ICoreObjectsModule::get() { return ModuleManager::get()->getOrLoadModulePtr<ICoreObjectsModule>(TCHAR("CoreObjects")); }

void CoreObjectsModule::init()
{
    cbe::initializeObjectAllocators();
    CoreObjectDelegates::onContentDirectoryAdded.bindObject(&packMan, &CBEPackageManager::registerContentRoot);
    CoreObjectDelegates::onContentDirectoryRemoved.bindObject(&packMan, &CBEPackageManager::registerContentRoot);
    CoreObjectDelegates::onObjectDestroyed.bindObject(&packMan, &CBEPackageManager::onObjectDeleted);
}

void CoreObjectsModule::release()
{
    CoreObjectDelegates::onContentDirectoryAdded.unbindAll(&packMan);
    CoreObjectDelegates::onContentDirectoryRemoved.unbindAll(&packMan);
    CoreObjectDelegates::onObjectDestroyed.unbindAll(&packMan);
}

const CoreObjectsDB &CoreObjectsModule::getObjectsDB() const { return objsDb; }

cbe::Package *CoreObjectsModule::getTransientPackage() const { return cbe::getDefaultObject<cbe::Package>(); }

CoreObjectGC &CoreObjectsModule::getGC() { return gc; }
