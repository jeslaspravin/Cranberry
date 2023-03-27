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
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/Threading/CoPaT/CoroutineWait.h"
#include "RenderInterface/IRHIModule.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderApi/RenderManager.h"

DECLARE_MODULE(EngineRenderer, EngineRendererModule)

//////////////////////////////////////////////////////////////////////////
/// Rendering thread stubs
//////////////////////////////////////////////////////////////////////////

copat::NormalFuncAwaiter
initializeGraphicsStub(RenderManager *renderManager, IGraphicsInstance *graphicsInstanceCache, const GraphicsHelperAPI *graphicsHelper)
{
    co_await renderManager->initialize(graphicsInstanceCache, graphicsHelper);
}

//////////////////////////////////////////////////////////////////////////
/// EngineRendererModule implementations
//////////////////////////////////////////////////////////////////////////

IGraphicsInstance *EngineRendererModule::currentGraphicsInstance() const
{
    ASSERT_INSIDE_RENDERTHREAD();
    return graphicsInstanceCache;
}

const GraphicsHelperAPI *EngineRendererModule::currentGraphicsHelper() const
{
    ASSERT_INSIDE_RENDERTHREAD();
    return graphicsHelperCache;
}

RenderManager *EngineRendererModule::getRenderManager() const
{
    ASSERT_INSIDE_RENDERTHREAD();
    return renderManager;
}

void EngineRendererModule::initializeGraphics(bool bComputeOnly /*= false*/)
{
    GlobalRenderVariables::GPU_IS_COMPUTE_ONLY.set(bComputeOnly);
    initializeGraphicsStub(renderManager, graphicsInstanceCache, graphicsHelperCache);
}

void EngineRendererModule::finalizeGraphicsInitialization() { renderManager->finalizeInit(); }

DelegateHandle EngineRendererModule::registerToStateEvents(RenderStateDelegate::SingleCastDelegateType &&callback)
{
    return renderStateEvents.bind(std::forward<RenderStateDelegate::SingleCastDelegateType>(callback));
}

void EngineRendererModule::unregisterToStateEvents(const DelegateHandle &handle) { renderStateEvents.unbind(handle); }

void EngineRendererModule::init()
{
    renderManager = new RenderManager();
    weakRHI = ModuleManager::get()->getOrLoadModule(TCHAR("VulkanRHI"));
    auto rhiModule = weakRHI.lock().get();

    graphicsInstanceCache = static_cast<IRHIModule *>(rhiModule)->createGraphicsInstance();
    graphicsHelperCache = static_cast<IRHIModule *>(rhiModule)->getGraphicsHelper();
}

void EngineRendererModule::release()
{
    if (GlobalRenderVariables::GPU_DEVICE_INITIALIZED)
    {
        // Wait till all graphics resources are released
        copat::waitOnAwaitable(renderManager->destroy());
    }
    renderManager = nullptr;
    delete renderManager;

    if (!weakRHI.expired())
    {
        auto rhiModule = weakRHI.lock().get();
        static_cast<IRHIModule *>(rhiModule)->destroyGraphicsInstance();
    }
    weakRHI.reset();

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

RenderThreadEnqTask RenderThreadEnqueuer::execInRenderThreadAwaitable(RenderThreadEnqueuer::RenderEnqFuncType execFunc)
{
    if (IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get())
    {
        execFunc(
            renderInterface->getRenderManager()->getRenderCmds(), renderInterface->currentGraphicsInstance(),
            renderInterface->currentGraphicsHelper()
        );
    }
    co_return;
}

void RenderThreadEnqueuer::execInRenderThreadAndWait(RenderEnqFuncType &&execFunc)
{
    if (copat::JobSystem::get()->isInThread(copat::EJobThreadType::RenderThread))
    {
        IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get();
        debugAssert(renderInterface);
        execFunc(
            renderInterface->getRenderManager()->getRenderCmds(), renderInterface->currentGraphicsInstance(),
            renderInterface->currentGraphicsHelper()
        );
        return;
    }
    copat::waitOnAwaitable(execInRenderThreadAwaitable(std::forward<RenderEnqFuncType>(execFunc)));
}

copat::NormalFuncAwaiter RenderThreadEnqueuer::execInRenderingThreadOrImmediate(RenderEnqFuncType &&execFunc)
{
    if (copat::JobSystem::get()->isInThread(copat::EJobThreadType::RenderThread))
    {
        IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get();
        debugAssert(renderInterface);
        execFunc(
            renderInterface->getRenderManager()->getRenderCmds(), renderInterface->currentGraphicsInstance(),
            renderInterface->currentGraphicsHelper()
        );
        co_return;
    }
    co_await execInRenderThreadAwaitable(std::forward<RenderEnqFuncType>(execFunc));
}