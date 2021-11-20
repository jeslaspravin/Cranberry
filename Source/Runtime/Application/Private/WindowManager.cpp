#include "WindowManager.h"
#include "PlatformInstances.h"
#include "Logger/Logger.h"
#include "Engine/Config/EngineGlobalConfigs.h"
#include "GenericAppWindow.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GraphicsIntance.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "ApplicationInstance.h"
#include "ApplicationModule.h"


GenericAppWindow* WindowManager::getMainWindow() const
{
    return appMainWindow;
}

void WindowManager::init()
{
    const ApplicationInstance* appInstance = IApplicationModule::get()->getApplication();
    appMainWindow = new PlatformAppWindow();

    appMainWindow->setWindowSize(EngineSettings::screenSize.get().x, EngineSettings::screenSize.get().y, false);
    appMainWindow->setWindowName(appInstance->getAppName());
    appMainWindow->setWindowMode(EngineSettings::fullscreenMode.get());
    appMainWindow->onWindowActivated.bindObject(this, &WindowManager::activateWindow, appMainWindow);
    appMainWindow->onWindowDeactived.bindObject(this, &WindowManager::deactivateWindow, appMainWindow);
    appMainWindow->onResize.bindObject(this, &WindowManager::onWindowResize, appMainWindow);
    appMainWindow->onDestroyRequested.bindObject(this, &WindowManager::requestDestroyWindow, appMainWindow);
    appMainWindow->createWindow(appInstance);
    static_cast<ApplicationModule*>(IApplicationModule::get())->onWindowCreated.invoke(appMainWindow);

    ENQUEUE_COMMAND(MainWindowInit)(
        [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            ManagerData& data = windowsOpened[appMainWindow];
            data.windowCanvas = graphicsHelper->createWindowCanvas(graphicsInstance, appMainWindow);
            data.windowCanvas->init();
            // Surface created at this point so cache the surface properties in Rendering system
            graphicsHelper->cacheSurfaceProperties(graphicsInstance, data.windowCanvas);
        });

}

void WindowManager::destroy()
{
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        WindowCanvasRef windowCanvas = windowData.second.windowCanvas;
        ENQUEUE_COMMAND(MainWindowDestroy)(
            [windowCanvas](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
            {
                windowCanvas->release();
            });
        windowData.second.windowCanvas.reset();

        static_cast<ApplicationModule*>(IApplicationModule::get())->onWindowDestroyed.invoke(windowData.first);
        windowData.first->destroyWindow();
        delete windowData.first;
    }
    appMainWindow = nullptr;
    windowsOpened.clear();
}

void WindowManager::destroyWindow(GenericAppWindow* window)
{
    if (window && window->isValidWindow())
    {
        auto foundItr = windowsOpened.find(window);
        if (foundItr != windowsOpened.cend())
        {
            WindowCanvasRef windowCanvas = foundItr->second.windowCanvas;
            ENQUEUE_COMMAND(WindowDestroy)(
                [windowCanvas](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
                {
                    windowCanvas->release();
                });

            auto* appModule = static_cast<ApplicationModule*>(IApplicationModule::get());
            appModule->onWindowDestroyed.invoke(foundItr->first);
            foundItr->first->destroyWindow();
            delete foundItr->first;

            windowsOpened.erase(foundItr);

            if (windowsOpened.empty())
            {
                appModule->onAllWindowsDestroyed.invoke();
            }
        }
    }
}

WindowCanvasRef WindowManager::getWindowCanvas(GenericAppWindow* window) const
{
    if (window && window->isValidWindow())
    {
        auto foundItr = windowsOpened.find(window);
        return foundItr != windowsOpened.cend() ? foundItr->second.windowCanvas : nullptr;
    }
    return nullptr;
}

void WindowManager::postInitGraphicCore()
{
    ENQUEUE_COMMAND(InitWindowCanvas)(
        [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
            {
                // As init might have failed when preparing initial surface
                windowData.second.windowCanvas->reinitResources();
            }
            EngineSettings::surfaceSize.set(Size2D(appMainWindow->windowWidth, appMainWindow->windowHeight));
        });
}

void WindowManager::updateWindowCanvas()
{
    ENQUEUE_COMMAND(UpdateWindowCanvas)(
        [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            auto* appModule = static_cast<ApplicationModule*>(IApplicationModule::get());
            cmdList->flushAllcommands();
            for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
            {
                appModule->preWindowSurfaceUpdate.invoke(windowData.first);
                windowData.second.windowCanvas->reinitResources();
                appModule->onWindowSurfaceUpdated.invoke(windowData.first);
            }
        });
}

void WindowManager::activateWindow(GenericAppWindow* window)
{
    if (window != activeWindow)
    {
        if (activeWindow != nullptr)
        {
            deactivateWindow(activeWindow);
        }
        activeWindow = window;
    }
}

void WindowManager::deactivateWindow(GenericAppWindow* window)
{
    if (window == activeWindow)
    {
        activeWindow = nullptr;
    }
}

bool WindowManager::pollWindows()
{
    windowsToDestroy.clear();
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        windowData.first->updateWindow();
    }

    std::vector<WindowCanvasRef> canvasesToDestroy;
    canvasesToDestroy.reserve(windowsToDestroy.size());
    auto* appModule = static_cast<ApplicationModule*>(IApplicationModule::get());
    for (GenericAppWindow* window : windowsToDestroy)
    {
        auto foundItr = windowsOpened.find(window);

        WindowCanvasRef windowCanvas = foundItr->second.windowCanvas;
        canvasesToDestroy.emplace_back(windowCanvas);

        appModule->onWindowDestroyed.invoke(foundItr->first);
        foundItr->first->destroyWindow();
        delete foundItr->first;

        windowsOpened.erase(foundItr);

        if (windowsOpened.empty())
        {
            appModule->onAllWindowsDestroyed.invoke();
        }
    }
    if (!canvasesToDestroy.empty())
    {
        ENQUEUE_COMMAND(WindowsCanvasDestroy)(
            [canvasesToDestroy](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper) mutable
            {
                // Clear will release and destroy ref resources
                canvasesToDestroy.clear();
            });
    }

    return activeWindow != nullptr;
}

void WindowManager::onWindowResize(uint32 width, uint32 height, GenericAppWindow* window)
{
    if (window->windowHeight != height || window->windowWidth != width)
    {
        ENQUEUE_COMMAND(WindowResize)(
            [this, window, width, height](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
            {
                ApplicationModule* appModule = static_cast<ApplicationModule*>(IApplicationModule::get());

                cmdList->flushAllcommands();
                appModule->preWindowSurfaceUpdate.invoke(window);

                window->setWindowSize(width, height, true);
                // Update the canvas
                WindowCanvasRef windowCanvas = getWindowCanvas(window);
                if (windowCanvas.isValid())
                {
                    Logger::debug("WindowsAppWindow", "%s() : Reiniting window canvas", __func__);
                    windowCanvas->reinitResources();
                }

                appModule->onWindowSurfaceUpdated.invoke(window);
                if (window == appMainWindow)
                {
                    Size2D newSize{ window->windowWidth, window->windowHeight };
                    EngineSettings::surfaceSize.set(newSize);
                }
            });
    }
}

void WindowManager::requestDestroyWindow(GenericAppWindow* window)
{
    windowsToDestroy.emplace(window);
}
