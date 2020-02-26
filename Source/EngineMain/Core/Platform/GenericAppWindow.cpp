#include "GenericAppWindow.h"



void GenericAppWindow::windowSize(uint32& width, uint32& height) const
{
    width = windowWidth;
    height = windowHeight;
}

void GenericAppWindow::setWindowSize(const uint32& width, const uint32& height, bool updateResources)
{
    windowWidth = width;
    windowHeight = height;
    if(updateResources)
        resizeWindow();
}

void GenericAppWindow::setWindowName(const String& wndName)
{
    windowName = wndName;
}

void GenericAppWindow::destroyWindow()
{
    for (GenericAppWindow* child : childWindows)
    {
        child->destroyWindow();
    }
    childWindows.clear();
}
