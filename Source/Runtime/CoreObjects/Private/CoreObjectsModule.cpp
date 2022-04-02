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

ICoreObjectsModule* ICoreObjectsModule::get()
{
    static WeakModulePtr weakRiModule = (ModuleManager::get()->getOrLoadModule(TCHAR("CoreObjects")));
    return weakRiModule.expired() ? nullptr : static_cast<CoreObjectsModule*>(weakRiModule.lock().get());
}

void CoreObjectsModule::init()
{
    CBE::initializeObjectAllocators();
}

void CoreObjectsModule::release()
{

}

const CoreObjectsDB& CoreObjectsModule::getObjectsDB() const
{
    throw std::logic_error("The method or operation is not implemented.");
}
