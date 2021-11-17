#include "EngineRendererModule.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/IRHIModule.h"

DECLARE_MODULE(EngineRenderer, EngineRedererModule)

IGraphicsInstance* EngineRedererModule::currentGraphicsInstance() const
{
    debugAssert(getRenderManager()->isExecutingCommands() && "using graphics instance any where outside render commands is not allowed");
    return graphicsInstanceCache;
}

const GraphicsHelperAPI* EngineRedererModule::currentGraphicsHelper() const 
{
    return graphicsHelperCache;
}

void EngineRedererModule::initializeGraphics()
{
    getRenderManager()->initialize(graphicsInstanceCache);
}

void EngineRedererModule::finalizeGraphicsInitialization()
{
    getRenderManager()->finalizeInit();
}

RenderManager* EngineRedererModule::getRenderManager() const
{
    static RenderManager renderManager;
    return &renderManager;
}

DelegateHandle EngineRedererModule::registerToStateEvents(RenderStateDelegate::SingleCastDelegateType callback)
{
    return renderStateEvents.bind(callback);
}

void EngineRedererModule::unregisterToStateEvents(const DelegateHandle& handle)
{
    renderStateEvents.unbind(handle);
}

void EngineRedererModule::init()
{
    weakRHI = ModuleManager::get()->getOrLoadModule("VulkanRHI");
    auto rhiModule = weakRHI.lock();

    graphicsInstanceCache = static_cast<IRHIModule*>(rhiModule.get())->createGraphicsInstance();
    graphicsHelperCache = static_cast<IRHIModule*>(rhiModule.get())->getGraphicsHelper();
}

void EngineRedererModule::release()
{
    getRenderManager()->destroy();

    WeakModulePtr weakRHI = ModuleManager::get()->getOrLoadModule("VulkanRHI");
    auto rhiModule = weakRHI.lock();

    static_cast<IRHIModule*>(rhiModule.get())->destroyGraphicsInstance();
    graphicsInstanceCache = nullptr;
    graphicsHelperCache = nullptr;
}

// IRenderInterfaceModule Impl

IRenderInterfaceModule* IRenderInterfaceModule::get()
{
    static WeakModulePtr weakRiModule = (ModuleManager::get()->getOrLoadModule("EngineRenderer"));
    return weakRiModule.expired() ? nullptr : static_cast<IRenderInterfaceModule*>(weakRiModule.lock().get());
}