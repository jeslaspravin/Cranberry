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
#include "WindowManager.h"
#include "GenericAppWindow.h"
#include "Widgets/WidgetWindow.h"
#include "ApplicationSettings.h"
#include "InputSystem/InputSystem.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"

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
    windowWidgets.erase(appWindow);
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
            Short2D mouseAbsPos = Short2D(int16(mouseScreenPos.x / scaleFactor), int16(mouseScreenPos.y / scaleFactor));
            lastHoverWnd->mouseLeave(mouseAbsPos, lastHoverWnd->screenToWindowSpace(mouseScreenPos), inputSystem);
        }
        lastHoverWnd = wndWidget;
        if (lastHoverWnd)
        {
            float scaleFactor = lastHoverWnd->getWidgetScaling();
            Short2D mouseAbsPos = Short2D(int16(mouseScreenPos.x / scaleFactor), int16(mouseScreenPos.y / scaleFactor));
            lastHoverWnd->mouseEnter(mouseAbsPos, lastHoverWnd->screenToWindowSpace(mouseScreenPos), inputSystem);
        }
    }
    if (lastHoverWnd && (screenMouseX->acceleration != 0.0f || screenMouseY->acceleration != 0.0f))
    {
        float scaleFactor = lastHoverWnd->getWidgetScaling();
        Short2D mouseAbsPos = Short2D(int16(mouseScreenPos.x / scaleFactor), int16(mouseScreenPos.y / scaleFactor));
        lastHoverWnd->mouseMoved(mouseAbsPos, lastHoverWnd->screenToWindowSpace(mouseScreenPos), inputSystem);
    }

    for (const std::pair<GenericAppWindow *const, SharedPtr<WgWindow>> window : windowWidgets)
    {
        if (window.first->isValidWindow() && !window.first->isMinimized())
        {
            window.second->tick(timeData.getDeltaTime());
        }
    }
}

void ApplicationInstance::drawWindowWidgets()
{
    std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> allDrawCtxs;
    allDrawCtxs.reserve(windowWidgets.size());
    for (const std::pair<GenericAppWindow *const, SharedPtr<WgWindow>> window : windowWidgets)
    {
        if (window.first->isValidWindow() && !window.first->isMinimized())
        {
            auto &wndWidgetDrawContext = allDrawCtxs.emplace_back();
            wndWidgetDrawContext.first = window.second;
            window.second->drawWidget(wndWidgetDrawContext.second);
        }
    }

    ENQUEUE_COMMAND(DrawWindowWidgets)
    (
        [allDrawCtxs = std::move(allDrawCtxs),
         this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            drawWindowWidgetsRenderThread(allDrawCtxs, cmdList, graphicsInstance, graphicsHelper);
        }
    );
}

void ApplicationInstance::drawWindowWidgetsRenderThread(
    const std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> &drawingContexts, IRenderCommandList *cmdList,
    IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
) const
{
    // TODO(Jeslas): Draw widgets data
}