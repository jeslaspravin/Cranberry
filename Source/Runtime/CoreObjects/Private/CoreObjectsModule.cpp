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

DECLARE_MODULE(CoreObjects, CoreObjectsModule)

ICoreObjectsModule *ICoreObjectsModule::get()
{
    static WeakModulePtr weakRiModule = (ModuleManager::get()->getOrLoadModule(TCHAR("CoreObjects")));
    return weakRiModule.expired() ? nullptr : static_cast<CoreObjectsModule *>(weakRiModule.lock().get());
}

void CoreObjectsModule::init() { CBE::initializeObjectAllocators(); }

void CoreObjectsModule::release() {}

const CoreObjectsDB &CoreObjectsModule::getObjectsDB() const { return objsDb; }

CoreObjectGC &CoreObjectsModule::getGC() { return gc; }
