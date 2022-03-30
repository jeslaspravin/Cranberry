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
#include "Modules/ModuleManager.h"
#include "CoreObjectAllocator.h"

DECLARE_MODULE(CoreObjects, CoreObjectsModule)

void CoreObjectsModule::init()
{
    CBE::initializeObjectAllocators();
}

void CoreObjectsModule::release()
{

}

CoreObjectsModule* CoreObjectsModule::get()
{
    static WeakModulePtr weakRiModule = (ModuleManager::get()->getOrLoadModule(TCHAR("CoreObjects")));
    return weakRiModule.expired() ? nullptr : static_cast<CoreObjectsModule*>(weakRiModule.lock().get());
}
