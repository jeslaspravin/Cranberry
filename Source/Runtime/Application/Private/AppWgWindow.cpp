/*!
 * \file AppWgWindow.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "IApplicationModule.h"
#include "ApplicationInstance.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "WindowManager.h"
#include "GenericAppWindow.h"
#include "Widgets/WidgetDrawContext.h"
#include "Widgets/NullWidget.h"
#include "Widgets/WidgetWindow.h"
#include "Widgets/WidgetRenderer.h"
#include "ApplicationSettings.h"
#include "InputSystem/InputSystem.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderApi/RenderManager.h"
#include "IRenderInterfaceModule.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "Profiler/ProgramProfiler.hpp"

//////////////////////////////////////////////////////////////////////////
/// WgWindow Implementations
//////////////////////////////////////////////////////////////////////////

float WgWindow::getWidgetScaling() const { return ownerWindow->dpiScale() * scaling; }

Short2D WgWindow::getWidgetSize() const
{
    uint32 width, height;
    ownerWindow->windowSize(width, height);
    return removeDpiScale(Short2D(width, height));
}

Short2D WgWindow::screenToWgWindowSpace(Short2D screenPt) const
{
    Short2D windowOrigin = ownerWindow->windowClientRect().minBound;
    Short2D windowSpace = screenPt - windowOrigin;
    return removeDpiScale(windowSpace);
}

Short2D WgWindow::wgWindowToScreenSpace(Short2D windowPt) const
{
    Short2D screenSpace = applyDpiScale(windowPt);
    return screenSpace + ownerWindow->windowClientRect().minBound;
}

//////////////////////////////////////////////////////////////////////////
/// WidgetBase Implementations
//////////////////////////////////////////////////////////////////////////

SharedPtr<WgWindow> WidgetBase::findWidgetParentWindow(SharedPtr<WidgetBase> widget)
{
    ApplicationInstance *app = IApplicationModule::get()->getApplication();
    if (!widget || !app)
    {
        return nullptr;
    }
    SharedPtr<WidgetBase> rootWidget = widget;
    while (!rootWidget->parentWidget.expired())
    {
        rootWidget = rootWidget->parentWidget.lock();
    }

    if (app->isAWindow(rootWidget))
    {
        return std::static_pointer_cast<WgWindow>(rootWidget);
    }
    return app->findWidgetParentWindow(widget);
}

//////////////////////////////////////////////////////////////////////////
/// ApplicationInstance Implementations
//////////////////////////////////////////////////////////////////////////

copat::NormalFuncAwaiter enqExitApp()
{
    co_await copat::SwitchJobThreadAwaiter<copat::EJobThreadType::MainThread>{};
    IApplicationModule::get()->getApplication()->requestExit();
}
void ApplicationInstance::exitNextFrame() { enqExitApp(); }

StackAllocator<EThreadSharing::ThreadSharing_Exclusive> &ApplicationInstance::getFrameAllocator()
{
    debugAssert(copat::JobSystem::get()->isInThread(copat::EJobThreadType::MainThread));
    return frameAllocator;
}

StackAllocator<EThreadSharing::ThreadSharing_Exclusive> &ApplicationInstance::getRenderFrameAllocator()
{
    ASSERT_INSIDE_RENDERTHREAD();
    return renderFrameAllocator;
}

SharedPtr<WgWindow> ApplicationInstance::getMainWindow() const
{
    if (ApplicationSettings::computeOnly)
    {
        return nullptr;
    }

    if (ApplicationSettings::renderingOffscreen)
    {
        // TODO(Jeslas) : Add off screen proxy window
        return nullptr;
    }
    else
    {
        debugAssert(windowManager);
        auto widgetItr = windowWidgets.find(windowManager->getMainWindow());
        debugAssert(widgetItr != windowWidgets.cend());
        return widgetItr->second;
    }
}

WindowCanvasRef ApplicationInstance::getWindowCanvas(SharedPtr<WgWindow> window) const
{
    if (ApplicationSettings::computeOnly)
    {
        return nullptr;
    }

    if (ApplicationSettings::renderingOffscreen)
    {
        // TODO(Jeslas) : Add off screen proxy window canvas
        return nullptr;
    }
    else
    {
        auto widgetItr = windowWidgets.find(window->getAppWindow());
        debugAssert(windowManager && widgetItr != windowWidgets.cend());
        return windowManager->getWindowCanvas(widgetItr->first);
    }
}

SharedPtr<WgWindow> ApplicationInstance::getActiveWindow() const
{
    if (ApplicationSettings::renderingOffscreen || ApplicationSettings::computeOnly)
    {
        // TODO(Jeslas) : Add off screen proxy window
        return nullptr;
    }
    return windowWidgets.find(windowManager->getActiveWindow())->second;
}

bool ApplicationInstance::hasActiveWindow() const { return ApplicationSettings::renderingOffscreen || windowManager->hasActiveWindow(); }

SharedPtr<WgWindow> ApplicationInstance::createWindow(Size2D size, const TChar *name, SharedPtr<WgWindow> parent)
{
    if (ApplicationSettings::renderingOffscreen || ApplicationSettings::computeOnly)
    {
        LOG_ERROR("ApplicationInstance", "Window(%s) creation not allowed in this application %s", name, applicationName);
        return nullptr;
    }
    fatalAssertf(jobSystem->isInThread(copat::EJobThreadType::MainThread), "Windows[%s] should be created or destroyed from main thread", name);

    GenericAppWindow *window = windowManager->createWindow(size, name, parent ? parent->getAppWindow() : nullptr);
    SharedPtr<WgWindow> windowWidget = createWindowWidget(window);
    windowWidgets[window] = windowWidget;
    return windowWidget;
}

void ApplicationInstance::destroyWindow(SharedPtr<WgWindow> window)
{
    fatalAssertf(
        jobSystem->isInThread(copat::EJobThreadType::MainThread), "Windows[%s] should be created or destroyed from main thread",
        window->getAppWindow()->getWindowName()
    );
    debugAssert(window && window->getAppWindow());
    windowWidgets.erase(window->getAppWindow());
    windowManager->destroyWindow(window->getAppWindow());
}

bool ApplicationInstance::isAWindow(SharedPtr<WidgetBase> widget)
{
    for (const auto &windowWidget : windowWidgets)
    {
        if (windowWidget.second == widget)
        {
            return true;
        }
    }
    return false;
}
SharedPtr<WgWindow> ApplicationInstance::findWidgetParentWindow(SharedPtr<WidgetBase> widget)
{
    for (const auto &windowWidget : windowWidgets)
    {
        if (windowWidget.second->hasWidget(widget))
        {
            return windowWidget.second;
        }
    }
    for (const auto &windowWidget : windowWidgets)
    {
        windowWidget.second->rebuildWindowGeoms();
        WidgetGeom geom = windowWidget.second->findWidgetGeom(widget);
        if (geom.widget == widget)
        {
            return windowWidget.second;
        }
    }
    return nullptr;
}

SharedPtr<WgWindow> ApplicationInstance::createWindowWidget(GenericAppWindow *appWindow) const
{
    if (appWindow == nullptr)
    {
        return nullptr;
    }

    WgWindow::WgArguments args;
    args.content = nullptr;
    args.ownerWindow = appWindow;
    auto windowWidget = std::make_shared<WgWindow>();
    windowWidget->construct(args);
    return windowWidget;
}

void ApplicationInstance::onWindowDestroyed(GenericAppWindow *appWindow)
{
    // It is okay if this->destroyWindow() calls this or Window exit calls this
    if (lastHoverWnd && appWindow == lastHoverWnd->getAppWindow())
    {
        lastHoverWnd = nullptr;
    }
    auto itr = windowWidgets.find(appWindow);
    if (itr != windowWidgets.cend())
    {
        wgRenderer->clearWindowState(itr->second);
        itr->second->clearWindow();
        windowWidgets.erase(itr);
    }
}

void ApplicationInstance::tickWindowWidgets()
{
    if (hasActiveWindow())
    {
        SharedPtr<WgWindow> window = getActiveWindow();
        debugAssert(window);

        for (Keys::StateKeyType key : Keys::Range())
        {
            const KeyState *state = inputSystem->keyState(*key);
            if (state->keyWentDown || state->keyWentUp)
            {
                // We do not want to pass input keys to inactive/not focused window
                window->inputKey(key, *state, inputSystem);
            }
        }
        for (AnalogStates::StateKeyType key : AnalogStates::Range())
        {
            const AnalogStates::StateInfoType *state = inputSystem->analogState(key);
            if (state->acceleration != 0.0f || state->currentValue != 0.0f)
            {
                window->analogKey(key, *state, inputSystem);
            }
        }

        const AnalogStates::StateInfoType *screenMouseX = inputSystem->analogState(AnalogStates::AbsMouseX);
        const AnalogStates::StateInfoType *screenMouseY = inputSystem->analogState(AnalogStates::AbsMouseY);
        Short2D mouseScreenPos(int16(screenMouseX->currentValue), int16(screenMouseY->currentValue));
        GenericAppWindow *appWnd = windowManager->findWindowUnder(mouseScreenPos);
        SharedPtr<WgWindow> wndWidget = appWnd ? windowWidgets[appWnd] : nullptr;
        if (lastHoverWnd != wndWidget)
        {
            if (lastHoverWnd)
            {
                Short2D mouseAbsPos = lastHoverWnd->screenToWgWindowSpace(mouseScreenPos);
                lastHoverWnd->mouseLeave(mouseAbsPos, mouseAbsPos, inputSystem);
            }
            lastHoverWnd = wndWidget;
            if (lastHoverWnd)
            {
                Short2D mouseAbsPos = lastHoverWnd->screenToWgWindowSpace(mouseScreenPos);
                lastHoverWnd->mouseEnter(mouseAbsPos, mouseAbsPos, inputSystem);
            }
        }
        if (lastHoverWnd && (screenMouseX->acceleration != 0.0f || screenMouseY->acceleration != 0.0f))
        {
            Short2D mouseAbsPos = lastHoverWnd->screenToWgWindowSpace(mouseScreenPos);
            lastHoverWnd->mouseMoved(mouseAbsPos, mouseAbsPos, inputSystem);
        }
    }

    for (const std::pair<GenericAppWindow *const, SharedPtr<WgWindow>> window : windowWidgets)
    {
        if (window.first->isValidWindow() && !window.first->isMinimized())
        {
            window.second->tick(timeData.getDeltaTime());
        }
    }
}

std::vector<SharedPtr<WgWindow>> ApplicationInstance::drawWindowWidgets()
{
    std::vector<SharedPtr<WgWindow>> allDrawWindows;
    allDrawWindows.reserve(windowWidgets.size());
    for (const std::pair<GenericAppWindow *const, SharedPtr<WgWindow>> &window : windowWidgets)
    {
        if (window.first->isValidWindow() && !window.first->isMinimized())
        {
            allDrawWindows.emplace_back(window.second);
        }
    }

    return wgRenderer->drawWindowWidgets(allDrawWindows);
}

void ApplicationInstance::presentDrawnWnds(const std::vector<SharedPtr<WgWindow>> &windowsDrawn) { wgRenderer->presentWindows(windowsDrawn); }

void ApplicationInstance::clearWidgets()
{
    if (wgRenderer)
    {
        wgRenderer->destroy();
        // Will be deleted after destroyed in render thread
        wgRenderer = nullptr;
    }
    IApplicationModule::get()->unregisterOnWindowDestroyed(onWindowDestroyHandle);
    for (const std::pair<GenericAppWindow *const, SharedPtr<WgWindow>> &window : windowWidgets)
    {
        window.second->clearWindow();
    }
    windowWidgets.clear();
    lastHoverWnd.reset();
}

void ApplicationInstance::startNewFrame()
{
    frameAllocator.reset();

    /**
     * Flush wait until all previous render commands are finished,
     * This is to avoid over queuing render thread which happens as frame wait happens only in render thread
     * so main thread run wild and fills render queue with commands more than it can process
     */
    RenderThreadEnqueuer::execInRenderThreadAndWait(
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI * /*graphicsHelper*/)
        {
            CBE_PROFILER_MARKFRAME_N(CBE_PROFILER_CHAR("RenderFrame"));
            renderFrameAllocator.reset();
            IRenderInterfaceModule::get()->getRenderManager()->renderFrame(timeData.deltaTime);

            // Copy all shader updates from previous frame in this frame
            std::vector<BatchCopyBufferData> copies;
            std::vector<GraphicsResource *> shaderParams;
            ShaderParameters::staticType()->allRegisteredResources(shaderParams, true, true);
            for (GraphicsResource *resource : shaderParams)
            {
                static_cast<ShaderParameters *>(resource)->pullBufferParamUpdates(copies, cmdList, graphicsInstance);
                // Mostly update buffer/texture params will be empty as it will be handled in respective update codes
                static_cast<ShaderParameters *>(resource)->updateParams(cmdList, graphicsInstance);
            }
            if (!copies.empty())
            {
                cmdList->copyToBuffer(copies);
            }
        }
    );
}

//////////////////////////////////////////////////////////////////////////
/// Drawing window widget
//////////////////////////////////////////////////////////////////////////

std::vector<SharedPtr<WgWindow>> WidgetRenderer::drawWindowWidgets(const std::vector<SharedPtr<WgWindow>> &windows)
{
    std::vector<SharedPtr<WgWindow>> drawingWindows;
    drawingWindows.reserve(windows.size());
    std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> allDrawCtxs;
    allDrawCtxs.reserve(windows.size());
    for (const SharedPtr<WgWindow> &window : windows)
    {
        WidgetDrawContext wndDrawContext;
        window->drawWidget(wndDrawContext);
        if (!wndDrawContext.perVertexPos().empty())
        {
            allDrawCtxs.emplace_back(window, std::move(wndDrawContext));
            drawingWindows.emplace_back(window);
        }
    }

    if (!allDrawCtxs.empty())
    {
        drawWindowWidgets(std::move(allDrawCtxs));
    }
    return drawingWindows;
}

void WidgetRenderer::presentWindows(const std::vector<SharedPtr<WgWindow>> &windows)
{
    ApplicationInstance *app = IApplicationModule::get()->getApplication();
    WindowManager *windowsManager = app->windowManager;

    std::vector<WindowCanvasRef> allDrawSwapchains;
    allDrawSwapchains.reserve(windows.size());
    for (const SharedPtr<WgWindow> &window : windows)
    {
        allDrawSwapchains.emplace_back(windowsManager->getWindowCanvas(window->getAppWindow()));
    }

    if (!allDrawSwapchains.empty())
    {
        presentWindows(windows, allDrawSwapchains);
    }
}

APPLICATION_EXPORT SharedPtr<WgNullWidget> WgNullWidget::nullWidget = std::make_shared<WgNullWidget>();