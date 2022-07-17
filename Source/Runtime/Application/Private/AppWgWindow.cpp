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

#include "ApplicationInstance.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/Threading/CoPaT/CoroutineWait.h"
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

//////////////////////////////////////////////////////////////////////////
/// WgWindow Implementations
//////////////////////////////////////////////////////////////////////////

Short2D WgWindow::getWidgetSize() const
{
    uint32 width, height;
    ownerWindow->windowSize(width, height);
    return Short2D(int16(width / getWidgetScaling()), int16(height / getWidgetScaling()));
}

float WgWindow::getWidgetScaling() const { return ownerWindow->dpiScale() * scaling; }

Short2D WgWindow::screenToWindowSpace(Short2D screenPt) const
{
    Short2D windowOrigin = ownerWindow->windowClientRect().minBound;
    Short2D windowSpace = screenPt - windowOrigin;
    return Short2D(int16(windowSpace.x / getWidgetScaling()), int16(windowSpace.y / getWidgetScaling()));
}

Short2D WgWindow::windowToScreenSpace(Short2D windowPt) const
{
    Short2D screenSpace = Short2D(int16(windowPt.x * getWidgetScaling()), int16(windowPt.y * getWidgetScaling()));
    return screenSpace + ownerWindow->windowClientRect().minBound;
}

//////////////////////////////////////////////////////////////////////////
/// ApplicationInstance Implementations
//////////////////////////////////////////////////////////////////////////

StackAllocator<EThreadSharing::ThreadSharing_Exclusive> &ApplicationInstance::getFrameAllocator()
{
    debugAssert(copat::JobSystem::get()->getCurrentThreadType() == copat::EJobThreadType::MainThread);
    return frameAllocator;
}

StackAllocator<EThreadSharing::ThreadSharing_Exclusive> &ApplicationInstance::getRenderFrameAllocator()
{
    debugAssert(copat::JobSystem::get()->getCurrentThreadType() == copat::EJobThreadType::RenderThread);
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
    fatalAssertf(
        jobSystem->getCurrentThreadType() == copat::EJobThreadType::MainThread, "Windows[%s] should be created or destroyed from main thread",
        name
    );

    GenericAppWindow *window = windowManager->createWindow(size, name, parent ? parent->getAppWindow() : nullptr);
    SharedPtr<WgWindow> windowWidget = createWindowWidget(window);
    windowWidgets[window] = windowWidget;
    return windowWidget;
}

void ApplicationInstance::destroyWindow(SharedPtr<WgWindow> window)
{
    fatalAssertf(
        jobSystem->getCurrentThreadType() == copat::EJobThreadType::MainThread, "Windows[%s] should be created or destroyed from main thread",
        window->getAppWindow()->getWindowName()
    );
    debugAssert(window && window->getAppWindow());
    windowWidgets.erase(window->getAppWindow());
    windowManager->destroyWindow(window->getAppWindow());
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
        windowWidgets.erase(itr);
    }
}

void ApplicationInstance::tickWindowWidgets()
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

    const AnalogStates::StateInfoType *screenMouseX = inputSystem->analogState(AnalogStates::AbsMouseX);
    const AnalogStates::StateInfoType *screenMouseY = inputSystem->analogState(AnalogStates::AbsMouseY);
    Short2D mouseScreenPos(int16(screenMouseX->currentValue), int16(screenMouseY->currentValue));
    GenericAppWindow *appWnd = windowManager->findWindowUnder(mouseScreenPos);
    SharedPtr<WgWindow> wndWidget = appWnd ? windowWidgets[appWnd] : nullptr;
    if (lastHoverWnd != wndWidget)
    {
        if (lastHoverWnd)
        {
            float scaleFactor = lastHoverWnd->getWidgetScaling();
            Short2D mouseAbsPos = lastHoverWnd->screenToWindowSpace(mouseScreenPos);
            lastHoverWnd->mouseLeave(mouseAbsPos, mouseAbsPos, inputSystem);
        }
        lastHoverWnd = wndWidget;
        if (lastHoverWnd)
        {
            float scaleFactor = lastHoverWnd->getWidgetScaling();
            Short2D mouseAbsPos = lastHoverWnd->screenToWindowSpace(mouseScreenPos);
            lastHoverWnd->mouseEnter(mouseAbsPos, mouseAbsPos, inputSystem);
        }
    }
    if (lastHoverWnd && (screenMouseX->acceleration != 0.0f || screenMouseY->acceleration != 0.0f))
    {
        float scaleFactor = lastHoverWnd->getWidgetScaling();
        Short2D mouseAbsPos = lastHoverWnd->screenToWindowSpace(mouseScreenPos);
        lastHoverWnd->mouseMoved(mouseAbsPos, mouseAbsPos, inputSystem);
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

void ApplicationInstance::presentDrawnWnds(const std::vector<SharedPtr<WgWindow>> &windowsDrawn)
{
    std::vector<WindowCanvasRef> allDrawSwapchains;
    allDrawSwapchains.reserve(windowsDrawn.size());
    for (const SharedPtr<WgWindow> &window : windowsDrawn)
    {
        allDrawSwapchains.emplace_back(windowManager->getWindowCanvas(window->getAppWindow()));
    }

    if (!allDrawSwapchains.empty())
    {
        ENQUEUE_COMMAND(PresentAllWindows)
        (
            [allDrawSwapchains](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                std::vector<uint32> swapchainsIdxs(allDrawSwapchains.size());
                for (uint32 i = 0; i < allDrawSwapchains.size(); ++i)
                {
                    uint32 idx = allDrawSwapchains[i]->currentImgIdx();
                    swapchainsIdxs[i] = idx;
                }
                cmdList->presentImage(allDrawSwapchains, swapchainsIdxs, {});
            }
        );
    }
}

void ApplicationInstance::startNewFrame()
{
    frameAllocator.reset();

    /**
     * Flush wait until all previous render commands are finished, 
     * This is to avoid over queuing render thread which happens as frame wait happens only in render thread
     * so main thread run wild and fills render queue with commands more than it can process
     */
    copat::waitOnAwaitable(RenderThreadEnqueuer::execInRenderThreadAwaitable
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            renderFrameAllocator.reset();
            IRenderInterfaceModule::get()->getRenderManager()->renderFrame(timeData.deltaTime);
        }
    ));
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

APPLICATION_EXPORT SharedPtr<WgNullWidget> WgNullWidget::nullWidget = std::make_shared<WgNullWidget>();