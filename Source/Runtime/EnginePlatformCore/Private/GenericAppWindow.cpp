#include "GenericAppWindow.h"
#include "Logger/Logger.h"
#include "Engine/GameEngine.h"

void GenericAppWindow::updateWindowResources()
{
    // Nothing to do here
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
        updateWindowResources();
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
