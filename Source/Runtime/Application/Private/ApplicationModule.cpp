/*!
 * \file ApplicationModule.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ApplicationModule.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Modules/ModuleManager.h"
#include "CmdLine/CmdLine.h"
#include "PlatformInstances.h"
#include "ApplicationSettings.h"
#include "ApplicationInstance.h"
#include "FontManager.h"

DECLARE_MODULE(Application, ApplicationModule)

IApplicationModule *IApplicationModule::get()
{
    static WeakModulePtr appModule = ModuleManager::get()->getOrLoadModule(TCHAR("Application"));
    if (appModule.expired())
    {
        return nullptr;
    }
    return static_cast<IApplicationModule *>(appModule.lock().get());
}

void ApplicationModule::graphicsInitEvents(ERenderStateEvent renderState)
{
    if (!ApplicationSettings::renderingOffscreen.get() && getApplication()->windowManager)
    {
        switch (renderState)
        {
        case ERenderStateEvent::PostLoadInstance:
            /**
             * Init needs to be called at pre-init of graphics device so that main window will be created and its surface can be used to cache
             * present queues in GraphicsDevice, And since we need it to created in main thread we go with PostLoadInstance
             */
            windowMan.init();
            break;
        case ERenderStateEvent::PostInitDevice:
            /**
             * Post init ensures that windows created before init of graphics device has a chance to create/retrieve swapchain images
             */
            windowMan.postInitGraphicCore();
            break;
        case ERenderStateEvent::Cleanup:
            windowMan.destroy();
            break;
        default:
            break;
        }
    }
}

void ApplicationModule::startAndRun(ApplicationInstance *appInst, const AppInstanceCreateInfo &appCI)
{
    // Load core if not loaded already
    bool bCoreModulesLoaded = ModuleManager::get()->loadModule(TCHAR("ProgramCore"));
    fatalAssertf(bCoreModulesLoaded, "Loading core modules failed");
    // Needs to be parsed asap
    if (!ProgramCmdLine::get()->parse(appInst->getCmdLine()))
    {
        LOG_ERROR("Engine", "Invalid command line");
        ProgramCmdLine::get()->printCommandLine();
    }

    FontManager fontManager;
    PlatformAppInstance platformApp(appCI.platformAppHandle);
    appInst->platformApp = &platformApp;
    appInstance = appInst;

    // Initialize job system
    PlatformThreadingFunctions::printSystemThreadingInfo();
    copat::JobSystem jobSys;
    jobSys.initialize(
        copat::JobSystem::MainThreadTickFunc::createLambda(
            [&jobSys](void *appModulePtr)
            {
                ApplicationModule *appModule = (ApplicationModule *)appModulePtr;
                if (!appModule->appInstance->appTick())
                {
                    jobSys.exitMain();
                }
            }
        ),
        this
    );
    appInstance->jobSystem = &jobSys;

    IRenderInterfaceModule *engineRenderer = nullptr;
    // Initialize GPU device and renderer module if needed
    if (appCI.bUseGpu)
    {
        WeakModulePtr renderModule = ModuleManager::get()->getOrLoadModule(TCHAR("EngineRenderer"));
        fatalAssertf(!renderModule.expired(), "EngineRenderer not found!");

        if (!(appCI.bRenderOffscreen || appCI.bIsComputeOnly))
        {
            appInstance->windowManager = &windowMan;
            appInstance->inputSystem = &inputSystem;
        }
        // Since we could technically use font manager in compute only mode as well
        fontManager = FontManager(InitType_DefaultInit);
        appInstance->fontManager = &fontManager;

        engineRenderer = static_cast<IRenderInterfaceModule *>(renderModule.lock().get());
        // Registering before initialization to allow application for handle renderer events
        engineRenderer->registerToStateEvents(
            RenderStateDelegate::SingleCastDelegateType::createObject(appInstance, &ApplicationInstance::onRendererStateEvent)
        );

        engineRenderer->initializeGraphics(appCI.bIsComputeOnly);
    }
    else
    {
        appInstance->windowManager = nullptr;
        appInstance->inputSystem = nullptr;
        appInstance->fontManager = nullptr;
    }

    Logger::flushStream();
    LOG("Application", "%s application start", appCI.applicationName);
    appInstance->startApp();
    if (engineRenderer)
    {
        // Allow application to do some allowed renderer setups before finalizing initialization
        engineRenderer->finalizeGraphicsInitialization();
    }

    Logger::flushStream();
    jobSys.joinMain();

    LOG("Application", "%s application exit", appCI.applicationName);
    appInstance->exitApp();

    if (engineRenderer)
    {
        fontManager.clear();
        ModuleManager::get()->unloadModule(TCHAR("EngineRenderer"));
    }
    Logger::flushStream();

    // Shutdown the job system after renderer is released!
    jobSys.shutdown();

    appInstance = nullptr;
}

const ApplicationInstance *ApplicationModule::getApplication() const { return appInstance; }

void ApplicationModule::windowCreated(GenericAppWindow *createdWindow) const
{
    inputSystem.registerWindow(createdWindow);
    onWindowCreated.invoke(createdWindow);
}

DelegateHandle ApplicationModule::registerOnWindowCreated(AppWindowDelegate::SingleCastDelegateType callback)
{
    return onWindowCreated.bind(callback);
}

void ApplicationModule::unregisterOnWindowCreated(const DelegateHandle &callbackHandle) { onWindowCreated.unbind(callbackHandle); }

DelegateHandle ApplicationModule::registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType callback)
{
    return onPreWindowSurfaceUpdate.bind(callback);
}

void ApplicationModule::unregisterPreWindowSurfaceUpdate(const DelegateHandle &callbackHandle)
{
    onPreWindowSurfaceUpdate.unbind(callbackHandle);
}

DelegateHandle ApplicationModule::registerOnWindowSurfaceUpdated(AppWindowDelegate::SingleCastDelegateType callback)
{
    return onWindowSurfaceUpdated.bind(callback);
}

void ApplicationModule::unregisterOnWindowSurfaceUpdated(const DelegateHandle &callbackHandle)
{
    onWindowSurfaceUpdated.unbind(callbackHandle);
}

DelegateHandle ApplicationModule::registerOnWindowDestroyed(AppWindowDelegate::SingleCastDelegateType callback)
{
    return onWindowDestroyed.bind(callback);
}

void ApplicationModule::unregisterOnWindowDestroyed(const DelegateHandle &callbackHandle) { onWindowDestroyed.unbind(callbackHandle); }

void ApplicationModule::allWindowDestroyed() const
{
    if (appInstance)
    {
        appInstance->requestExit();
    }
    onAllWindowsDestroyed.invoke();
}

DelegateHandle ApplicationModule::registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType callback)
{
    return onAllWindowsDestroyed.bind(callback);
}

void ApplicationModule::unregisterAllWindowDestroyed(const DelegateHandle &callbackHandle) { onAllWindowsDestroyed.unbind(callbackHandle); }

void ApplicationModule::init()
{
    WeakModulePtr renderModule = ModuleManager::get()->getOrLoadModule(TCHAR("EngineRenderer"));
    if (!renderModule.expired())
    {
        graphicsInitEventHandle
            = static_cast<IRenderInterfaceModule *>(renderModule.lock().get())
                  ->registerToStateEvents(
                      RenderStateDelegate::SingleCastDelegateType::createObject(this, &ApplicationModule::graphicsInitEvents)
                  );
    }
}

void ApplicationModule::release()
{
    WeakModulePtr renderModule = ModuleManager::get()->getModule(TCHAR("EngineRenderer"));
    if (!renderModule.expired())
    {
        static_cast<IRenderInterfaceModule *>(renderModule.lock().get())->unregisterToStateEvents(graphicsInitEventHandle);
    }
    windowMan.destroy();
}
