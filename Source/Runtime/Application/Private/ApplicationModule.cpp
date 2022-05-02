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
#include "Modules/ModuleManager.h"
#include "PlatformInstances.h"

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
    switch (renderState)
    {
    case ERenderStateEvent::PreinitDevice:
        /**
         * Init needs to be called at pre-init of graphics device so that main window will be created and its surface can be used to cache
         * present queues in GraphicsDevice
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

ApplicationInstance *ApplicationModule::createApplication(const AppInstanceCreateInfo &appCI)
{
    appInstance = ApplicationInstance(appCI);
    appInstance.platformApp = new PlatformAppInstance(appCI.platformAppHandle);
    return &appInstance;
}

const ApplicationInstance *ApplicationModule::getApplication() const { return &appInstance; }

GenericAppWindow *ApplicationModule::mainWindow() { return windowMan.getMainWindow(); }

WindowManager *ApplicationModule::getWindowManager() { return &windowMan; }

bool ApplicationModule::pollWindows() { return windowMan.pollWindows(); }

DelegateHandle ApplicationModule::registerOnWindowCreated(AppWindowDelegate::SingleCastDelegateType callback)
{
    return onWindowCreated.bind(callback);
}

void ApplicationModule::unregisterOnWindowCreated(const DelegateHandle &callbackHandle) { onWindowCreated.unbind(callbackHandle); }

DelegateHandle ApplicationModule::registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType callback)
{
    return preWindowSurfaceUpdate.bind(callback);
}

void ApplicationModule::unregisterPreWindowSurfaceUpdate(const DelegateHandle &callbackHandle)
{
    preWindowSurfaceUpdate.unbind(callbackHandle);
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

DelegateHandle ApplicationModule::registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType callback)
{
    return onAllWindowsDestroyed.bind(callback);
}

void ApplicationModule::unregisterAllWindowDestroyed(const DelegateHandle &callbackHandle) { onAllWindowsDestroyed.unbind(callbackHandle); }

void ApplicationModule::init()
{
    graphicsInitEventHandle = IRenderInterfaceModule::get()->registerToStateEvents(
        RenderStateDelegate::SingleCastDelegateType::createObject(this, &ApplicationModule::graphicsInitEvents)
    );
}

void ApplicationModule::release()
{
    if (appInstance.platformApp)
    {
        delete appInstance.platformApp;
        appInstance.platformApp = nullptr;
    }
    windowMan.destroy();
    if (IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get())
    {
        renderInterface->unregisterToStateEvents(graphicsInitEventHandle);
    }
}
