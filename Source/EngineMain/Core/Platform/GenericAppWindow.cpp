#include "GenericAppWindow.h"
#include "../../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../Logger/Logger.h"
#include "../Engine/GameEngine.h"

void GenericAppWindow::resizeWindowResources()
{
    GenericWindowCanvas* windowCanvas = gEngine->getApplicationInstance()->appWindowManager.getWindowCanvas(this);
    if (windowCanvas)
    {
        Logger::debug("WindowsAppWindow", "%s() : Reiniting window canvas", __func__);
        windowCanvas->reinitResources();
    }
}

void GenericAppWindow::windowSize(uint32& width, uint32& height) const
{
    width = windowWidth;
    height = windowHeight;
}

void GenericAppWindow::setWindowSize(const uint32& width, const uint32& height, bool updateResources)
{
    windowWidth = width;
    windowHeight = height;
    if (updateResources)
    {
        resizeWindowResources();
    }
}

void GenericAppWindow::setWindowMode(bool bIsFullScreen)
{
    bIsWindowed = !bIsFullScreen;
}

void GenericAppWindow::setWindowName(const String& wndName)
{
    windowName = wndName;
}

void GenericAppWindow::updateWindow()
{
    for (const std::pair<const uint32, LambdaFunction<void>>& eventFunc : accumulatedEvents)
    {
        eventFunc.second();
    }
    accumulatedEvents.clear();
}

void GenericAppWindow::destroyWindow()
{
    for (GenericAppWindow* child : childWindows)
    {
        // TODO(Jeslas) : change this to call destroy in window manager as it needs to be updated there as well.
        child->destroyWindow();
    }
    childWindows.clear();
    onWindowDeactived.clear();
    onWindowActivated.clear();
}
