/*!
 * \file EngineRendererModule.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EngineRendererModule.h"
#include "Modules/ModuleManager.h"
#include "RenderInterface/IRHIModule.h"
#include "Types/Platform/PlatformAssertionErrors.h"

DECLARE_MODULE(EngineRenderer, EngineRedererModule)

IGraphicsInstance *EngineRedererModule::currentGraphicsInstance() const
{
    debugAssert(getRenderManager()->isExecutingCommands() && "using graphics instance any where outside render commands is not allowed");
    return graphicsInstanceCache;
}

const GraphicsHelperAPI *EngineRedererModule::currentGraphicsHelper() const { return graphicsHelperCache; }

void EngineRedererModule::initializeGraphics() { getRenderManager()->initialize(graphicsInstanceCache); }

void EngineRedererModule::finalizeGraphicsInitialization() { getRenderManager()->finalizeInit(); }

RenderManager *EngineRedererModule::getRenderManager() const { return renderManager; }

DelegateHandle EngineRedererModule::registerToStateEvents(RenderStateDelegate::SingleCastDelegateType callback)
{
    return renderStateEvents.bind(callback);
}

void EngineRedererModule::unregisterToStateEvents(const DelegateHandle &handle) { renderStateEvents.unbind(handle); }

void EngineRedererModule::init()
{
    renderManager = new RenderManager();
    weakRHI = ModuleManager::get()->getOrLoadModule(TCHAR("VulkanRHI"));
    auto rhiModule = weakRHI.lock().get();

    graphicsInstanceCache = static_cast<IRHIModule *>(rhiModule)->createGraphicsInstance();
    graphicsHelperCache = static_cast<IRHIModule *>(rhiModule)->getGraphicsHelper();
}

void EngineRedererModule::release()
{
    renderManager->destroy();
    delete renderManager;
    renderManager = nullptr;

    if (!weakRHI.expired())
    {
        auto rhiModule = weakRHI.lock().get();
        static_cast<IRHIModule *>(rhiModule)->destroyGraphicsInstance();
    }
    graphicsInstanceCache = nullptr;
    graphicsHelperCache = nullptr;
    ModuleManager::get()->unloadModule(TCHAR("VulkanRHI"));
}

// IRenderInterfaceModule Impl

IRenderInterfaceModule *IRenderInterfaceModule::get()
{
    static WeakModulePtr weakRiModule = (ModuleManager::get()->getOrLoadModule(TCHAR("EngineRenderer")));
    return weakRiModule.expired() ? nullptr : static_cast<IRenderInterfaceModule *>(weakRiModule.lock().get());
}