#include "EngineInputCoreModule.h"
#include "IApplicationModule.h"
#include "Modules/ModuleManager.h"

DECLARE_MODULE(EngineInputCore, EngineInputCoreModule)

void EngineInputCoreModule::createdNewWindow(GenericAppWindow* window) const
{
    inputSystem.registerWindow(window);
}

void EngineInputCoreModule::updateInputs()
{
    inputSystem.updateInputStates();
}

void EngineInputCoreModule::init()
{
    createdWindowHandle = IApplicationModule::get()
        ->registerOnWindowCreated(AppWindowDelegate::SingleCastDelegateType::createObject(this, &EngineInputCoreModule::createdNewWindow));
}

void EngineInputCoreModule::release()
{
    if (IApplicationModule* appModule = IApplicationModule::get())
    {
        appModule->unregisterOnWindowCreated(createdWindowHandle);
    }
}
