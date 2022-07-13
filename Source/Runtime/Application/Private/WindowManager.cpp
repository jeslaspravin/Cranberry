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
#include "RenderApi/Rendering/RenderingContexts.h"
#include "ApplicationSettings.h"
#include "IApplicationModule.h"

GenericAppWindow *WindowManager::getMainWindow() const { return appMainWindow; }

void WindowManager::init()
{
    const ApplicationInstance *appInstance = IApplicationModule::get()->getApplication();
    appMainWindow = new PlatformAppWindow();

    appMainWindow->setWindowSize(ApplicationSettings::screenSize.get().x, ApplicationSettings::screenSize.get().y, false);
    appMainWindow->setWindowName(appInstance->getAppName().getChar());
    appMainWindow->setWindowMode(ApplicationSettings::fullscreenMode.get());
    appMainWindow->onWindowActivated.bindObject(this, &WindowManager::activateWindow, appMainWindow);
    appMainWindow->onWindowDeactived.bindObject(this, &WindowManager::deactivateWindow, appMainWindow);
    appMainWindow->onResize.bindObject(this, &WindowManager::onWindowResize, appMainWindow);
    appMainWindow->onDestroyRequested.bindObject(this, &WindowManager::requestDestroyWindow, appMainWindow);
    windowsOpened[appMainWindow] = {};

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
    windowsToDestroy.clear();
    // Only parent destroys its child, If window is a child skip destroying
    for (std::pair<GenericAppWindow *const, ManagerData> &windowData : windowsOpened)
    {
        if (windowData.first->parentWindow == nullptr)
        {
            requestDestroyWindow(windowData.first);
        }
    }
    destroyPendingWindows();

    appMainWindow = nullptr;
    activeWindow = nullptr;
    windowsOpened.clear();
}

GenericAppWindow *WindowManager::createWindow(Size2D size, const TChar *name, GenericAppWindow *parent)
{
    const ApplicationInstance *appInstance = IApplicationModule::get()->getApplication();
    GenericAppWindow *appWindow = new PlatformAppWindow();

    appWindow->setWindowSize(size.x, size.y, false);
    appWindow->setWindowName(name);
    appWindow->setWindowMode(false);
    appWindow->setParent(parent ? parent : nullptr);
    appWindow->onWindowActivated.bindObject(this, &WindowManager::activateWindow, appWindow);
    appWindow->onWindowDeactived.bindObject(this, &WindowManager::deactivateWindow, appWindow);
    appWindow->onResize.bindObject(this, &WindowManager::onWindowResize, appWindow);
    appWindow->onDestroyRequested.bindObject(this, &WindowManager::requestDestroyWindow, appWindow);
    // We need order when creating so that auto activation will arrange windows
    ManagerData windowData{ .order = int32(windowsOpened.size()) };
    windowsOpened[appWindow] = windowData;

    appWindow->createWindow(appInstance);
    IApplicationModule::get()->windowCreated(appWindow);

    ENQUEUE_COMMAND(WindowInit)
    (
        [this, appWindow](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            ManagerData &data = windowsOpened[appWindow];
            data.windowCanvas = graphicsHelper->createWindowCanvas(graphicsInstance, appWindow);
            data.windowCanvas->init();
        }
    );
    return appWindow;
}

void WindowManager::destroyWindow(GenericAppWindow *window)
{
    deactivateWindow(window);
    windowsToDestroy.clear();
    requestDestroyWindow(window);
    destroyPendingWindows();
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

std::vector<GenericAppWindow *> WindowManager::getArrangedWindows() const
{
    std::vector<GenericAppWindow *> arrangedWindows(windowsOpened.size());
    for (const std::pair<GenericAppWindow *const, ManagerData> &wnd : windowsOpened)
    {
        arrangedWindows[wnd.second.order] = wnd.first;
    }
    return arrangedWindows;
}

GenericAppWindow *WindowManager::findWindowUnder(Short2D screenPos) const
{
    // First find using native API
    WindowHandle wndHnd = PlatformAppWindow::getWindowUnderPoint(screenPos);
    if (GenericAppWindow *appWnd = findNativeHandleWindow(wndHnd))
    {
        return appWnd;
    }

    std::vector<GenericAppWindow *> wnds = getArrangedWindows();
    for (GenericAppWindow *wnd : wnds)
    {
        if (wnd->isValidWindow() && !wnd->isMinimized() && wnd->windowRect().contains(screenPos))
        {
            return findChildWindowUnder(wnd, screenPos);
        }
    }
    return nullptr;
}

GenericAppWindow *WindowManager::findNativeHandleWindow(WindowHandle wndHnd) const
{
    if (wndHnd == nullptr)
        return nullptr;

    for (const std::pair<GenericAppWindow *const, ManagerData> &wnd : windowsOpened)
    {
        if (wnd.first->getWindowHandle() == wndHnd)
            return wnd.first;
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

        if (activeWindow)
        {
            // Rearrange windows
            ManagerData &activeWindowData = windowsOpened[activeWindow];
            int32 rearrangeBeforeOrder = activeWindowData.order;
            for (std::pair<GenericAppWindow *const, ManagerData> &wnd : windowsOpened)
            {
                wnd.second.order = (wnd.second.order >= rearrangeBeforeOrder) ? wnd.second.order : wnd.second.order + 1;
            }
            activeWindowData.order = 0;
        }
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
    destroyPendingWindows();
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
                if (windowCanvas.isValid() && !window->isMinimized())
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

void WindowManager::requestDestroyWindow(GenericAppWindow *window)
{
    // If destroying main window we have to destroy everything
    if (window == appMainWindow)
    {
        for (std::pair<GenericAppWindow *const, ManagerData> &windowData : windowsOpened)
        {
            if (windowData.first->parentWindow == nullptr && appMainWindow != windowData.first)
            {
                requestDestroyWindow(windowData.first);
            }
        }
    }

    for (GenericAppWindow *childWnd : window->childWindows)
    {
        requestDestroyWindow(childWnd);
    }
    windowsToDestroy.emplace_back(window);
}

void WindowManager::destroyPendingWindows()
{
    std::vector<WindowCanvasRef> canvasesToDestroy;
    canvasesToDestroy.reserve(windowsToDestroy.size());
    auto *appModule = IApplicationModule::get();
    for (GenericAppWindow *window : windowsToDestroy)
    {
        auto foundItr = windowsOpened.find(window);
        if (foundItr == windowsOpened.end())
        {
            // We removed this already
            continue;
        }

        ManagerData wndData = foundItr->second;
        canvasesToDestroy.emplace_back(wndData.windowCanvas);

        appModule->windowDestroyed(foundItr->first);
        foundItr->first->destroyWindow();
        delete foundItr->first;

        windowsOpened.erase(foundItr);
        // Rearrange windows after this window
        for (std::pair<GenericAppWindow *const, ManagerData> &wnd : windowsOpened)
        {
            // Nothing should have the old order
            debugAssert(wnd.second.order != wndData.order);
            wnd.second.order = (wnd.second.order < wndData.order) ? wnd.second.order : wnd.second.order - 1;
        }

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
                GlobalRenderingContextBase *renderingContext = IRenderInterfaceModule::get()->getRenderManager()->getGlobalRenderingContext();
                for (const WindowCanvasRef &windowCanvas : canvasesToDestroy)
                {
                    // Release the canvas related frame buffers before updating
                    renderingContext->clearWindowCanvasFramebuffer(windowCanvas);
                }
                // Clear will release and destroy ref resources
                canvasesToDestroy.clear();
            }
        );
    }
    windowsToDestroy.clear();
}

GenericAppWindow *WindowManager::findChildWindowUnder(GenericAppWindow *window, Short2D screenPos) const
{
    for (GenericAppWindow *childWnd : window->childWindows)
    {
        if (childWnd->isValidWindow() && !childWnd->isMinimized() && childWnd->windowRect().contains(screenPos))
        {
            return findChildWindowUnder(childWnd, screenPos);
        }
    }
    return window;
}
