/*!
 * \file GenericAppWindow.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "GenericAppWindow.h"
#include "Logger/Logger.h"

void GenericAppWindow::setWindowMode(bool bIsFullScreen) { bIsWindowed = !bIsFullScreen; }

void GenericAppWindow::setWindowName(const TChar *wndName) { windowName = wndName; }

void GenericAppWindow::setParent(GenericAppWindow *window)
{
    if (!window)
    {
        return;
    }
    if (isValidWindow())
    {
        LOG_ERROR("GenericAppWindow", "Cannot setup parent after window({}) is created!", windowName);
        return;
    }

    parentWindow = window;
    window->childWindows.emplace_back(this);
}

void GenericAppWindow::updateWindow()
{
    for (const std::pair<const uint32, LambdaFunction<void>> &eventFunc : accumulatedEvents)
    {
        eventFunc.second();
    }
    accumulatedEvents.clear();
}

void GenericAppWindow::destroyWindow()
{
    // Must have been destroyed before parent is destroyed
    debugAssertf(childWindows.empty(), "Child windows must be destroyed before parent {}", windowName);
    childWindows.clear();
    onWindowDeactived.clear();
    onWindowActivated.clear();
    onResize.clear();
    onDestroyRequested.clear();

    if (parentWindow)
    {
        std::erase(parentWindow->childWindows, this);
    }
}
