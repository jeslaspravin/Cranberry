/*!
 * \file EngineInputCoreModule.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EngineInputCoreModule.h"
#include "IApplicationModule.h"
#include "Modules/ModuleManager.h"

DECLARE_MODULE(EngineInputCore, EngineInputCoreModule)

void EngineInputCoreModule::createdNewWindow(GenericAppWindow *window) const
{
    inputSystem.registerWindow(window);
}

void EngineInputCoreModule::updateInputs() { inputSystem.updateInputStates(); }

void EngineInputCoreModule::init()
{
    createdWindowHandle = IApplicationModule::get()->registerOnWindowCreated(
        AppWindowDelegate::SingleCastDelegateType::createObject(
            this, &EngineInputCoreModule::createdNewWindow));
}

void EngineInputCoreModule::release()
{
    if (IApplicationModule *appModule = IApplicationModule::get())
    {
        appModule->unregisterOnWindowCreated(createdWindowHandle);
    }
}
