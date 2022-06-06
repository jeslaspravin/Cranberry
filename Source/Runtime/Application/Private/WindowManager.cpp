/*!
 * \file WindowManager.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WindowManager.h"
#include "PlatformInstances.h"
#include "ApplicationInstance.h"
#include "GenericAppWindow.h"
#include "Logger/Logger.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/RenderManager.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GraphicsIntance.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Rendering/RenderingContexts.h"
#include "ApplicationSettings.h"
#include "IApplicationModule.h"

GenericAppWindow *WindowManager::getMainWindow() const { return appMainWindow; }

void WindowManager::init()
{
    const ApplicationInstance *appInstance = IApplicationModule::get()->getApplication();
    appMainWindow = new PlatformAppWindow();

    appMainWindow->setWindowSize(ApplicationSettings::screenSize.get().x, ApplicationSettings::screenSize.get().y, false);
    appMainWindow->setWindowName(appInstance->getAppName());
    appMainWindow->setWindowMode(ApplicationSettings::fullscreenMode.get());
    appMainWindow->onWindowActivated.bindObject(this, &WindowManager::activateWindow, appMainWindow);
    appMainWindow->onWindowDeactived.bindObject(this, &WindowManager::deactivateWindow, appMainWindow);
    appMainWindow->onResize.bindObject(this, &WindowManager::onWindowResize, appMainWindow);
    appMainWindow->onDestroyRequested.bindObject(this, &WindowManager::requestDestroyWindow, appMainWindow);
    appMainWindow->createWindow(appInstance);
    IApplicationModule::get()->windowCreated(appMainWindow);

    ENQUEUE_COMMAND(MainWindowInit)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            ManagerData &data = windowsOpened[appMainWindow];
            data.windowCanvas = graphicsHelper->createWindowCanvas(graphicsInstance, appMainWindow);
            data.windowCanvas->init();
            // Surface created at this point so cache the surface properties in Rendering system
            graphicsHelper->cacheSurfaceProperties(graphicsInstance, data.windowCanvas);
        }
    );
}

void WindowManager::destroy()
{
    for (std::pair<GenericAppWindow *const, ManagerData> &windowData : windowsOpened)
    {
        WindowCanvasRef windowCanvas = windowData.second.windowCanvas;
        windowData.second.windowCanvas.reset();

        IApplicationModule::get()->windowDestroyed(windowData.first);
        windowData.first->destroyWindow();
        delete windowData.first;

        ENQUEUE_COMMAND(MainWindowDestroy)
        (
            [windowCanvas](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                // Release the canvas before deleting
                IRenderInterfaceModule::get()->getRenderManager()->getGlobalRenderingContext()->clearWindowCanvasFramebuffer(windowCanvas);
                windowCanvas->release();
            }
        );
    }
    appMainWindow = nullptr;
    windowsOpened.clear();
}

void WindowManager::destroyWindow(GenericAppWindow *window)
{
    if (window && window->isValidWindow())
    {
        auto foundItr = windowsOpened.find(window);
        if (foundItr != windowsOpened.cend())
        {
            WindowCanvasRef windowCanvas = foundItr->second.windowCanvas;
            ENQUEUE_COMMAND(WindowDestroy)
            ([windowCanvas](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
             { windowCanvas->release(); });

            auto *appModule = IApplicationModule::get();
            appModule->windowDestroyed(foundItr->first);
            foundItr->first->destroyWindow();
            delete foundItr->first;

            windowsOpened.erase(foundItr);

            if (windowsOpened.empty())
            {
                appModule->allWindowDestroyed();
            }
        }
    }
}

WindowCanvasRef WindowManager::getWindowCanvas(GenericAppWindow *window) const
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
    ENQUEUE_COMMAND(InitWindowCanvas)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            for (std::pair<GenericAppWindow *const, ManagerData> &windowData : windowsOpened)
            {
                // As init might have failed when preparing initial surface
                // And we do not need to free window canvas frame here as it is first init
                windowData.second.windowCanvas->reinitResources();
            }
            ApplicationSettings::surfaceSize.set(Size2D(appMainWindow->windowWidth, appMainWindow->windowHeight));
        }
    );
}

void WindowManager::updateWindowCanvas()
{
    ENQUEUE_COMMAND(UpdateWindowCanvas)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            auto *appModule = IApplicationModule::get();
            cmdList->flushAllcommands();
            for (std::pair<GenericAppWindow *const, ManagerData> &windowData : windowsOpened)
            {
                appModule->preWindowSurfaceUpdate(windowData.first);

                // Release the canvas related frame buffers before updating
                IRenderInterfaceModule::get()->getRenderManager()->getGlobalRenderingContext()->clearWindowCanvasFramebuffer(
                    windowData.second.windowCanvas
                );
                windowData.second.windowCanvas->reinitResources();
                appModule->windowSurfaceUpdated(windowData.first);
            }
        }
    );
}

void WindowManager::activateWindow(GenericAppWindow *window)
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

void WindowManager::deactivateWindow(GenericAppWindow *window)
{
    if (window == activeWindow)
    {
        activeWindow = nullptr;
    }
}

bool WindowManager::pollWindows()
{
    windowsToDestroy.clear();
    for (std::pair<GenericAppWindow *const, ManagerData> &windowData : windowsOpened)
    {
        windowData.first->updateWindow();
    }

    std::vector<WindowCanvasRef> canvasesToDestroy;
    canvasesToDestroy.reserve(windowsToDestroy.size());
    auto *appModule = IApplicationModule::get();
    for (GenericAppWindow *window : windowsToDestroy)
    {
        auto foundItr = windowsOpened.find(window);

        WindowCanvasRef windowCanvas = foundItr->second.windowCanvas;
        canvasesToDestroy.emplace_back(windowCanvas);

        appModule->windowDestroyed(foundItr->first);
        foundItr->first->destroyWindow();
        delete foundItr->first;

        windowsOpened.erase(foundItr);

        if (windowsOpened.empty())
        {
            appModule->allWindowDestroyed();
        }
    }
    if (!canvasesToDestroy.empty())
    {
        ENQUEUE_COMMAND(WindowsCanvasDestroy)
        (
            [canvasesToDestroy](
                class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
            ) mutable
            {
                cmdList->flushAllcommands();
                for (const WindowCanvasRef &windowCanvas : canvasesToDestroy)
                {
                    // Release the canvas related frame buffers before updating
                    IRenderInterfaceModule::get()->getRenderManager()->getGlobalRenderingContext()->clearWindowCanvasFramebuffer(windowCanvas);
                }
                // Clear will release and destroy ref resources
                canvasesToDestroy.clear();
            }
        );
    }

    return activeWindow != nullptr;
}

void WindowManager::onWindowResize(uint32 width, uint32 height, GenericAppWindow *window)
{
    if (window->windowHeight != height || window->windowWidth != width)
    {
        ENQUEUE_COMMAND(WindowResize)
        (
            [this, window, width,
             height](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                IApplicationModule *appModule = IApplicationModule::get();

                cmdList->flushAllcommands();
                appModule->preWindowSurfaceUpdate(window);

                window->setWindowSize(width, height, true);
                // Update the canvas
                WindowCanvasRef windowCanvas = getWindowCanvas(window);
                if (windowCanvas.isValid())
                {
                    LOG_DEBUG("WindowsAppWindow", "Reiniting window canvas");
                    // Release the canvas before updating
                    IRenderInterfaceModule::get()->getRenderManager()->getGlobalRenderingContext()->clearWindowCanvasFramebuffer(windowCanvas);
                    windowCanvas->reinitResources();
                }

                appModule->windowSurfaceUpdated(window);
                if (window == appMainWindow)
                {
                    Size2D newSize{ window->windowWidth, window->windowHeight };
                    ApplicationSettings::surfaceSize.set(newSize);
                }
            }
        );
    }
}

void WindowManager::requestDestroyWindow(GenericAppWindow *window) { windowsToDestroy.emplace(window); }
