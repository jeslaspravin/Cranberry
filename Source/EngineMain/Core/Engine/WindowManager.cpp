#include "WindowManager.h"
#include "../Platform/PlatformInstances.h"
#include "GameEngine.h"
#include "../../RenderInterface/PlatformIndependentHeaders.h"


GenericAppWindow* WindowManager::getMainWindow() const
{
    return appMainWindow;
}

void WindowManager::initMain()
{
    appMainWindow = new PlatformAppWindow();
    appMainWindow->setWindowSize(1280, 720, false);// TODO (Jeslas) : change this later
    appMainWindow->setWindowName(::gEngine->getAppName());
    appMainWindow->createWindow(::gEngine->getApplicationInstance());

    ManagerData& data = windowsOpened[appMainWindow];
    data.windowCanvas = new WindowCanvas();
    data.windowCanvas->setWindow(appMainWindow);
    data.windowCanvas->init();
}

void WindowManager::destroyMain()
{
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        windowData.second.windowCanvas->release();
        delete windowData.second.windowCanvas;
        windowData.second.windowCanvas = nullptr;

        windowData.first->destroyWindow();
        delete windowData.first;
    }
    appMainWindow = nullptr;
    windowsOpened.clear();
}

GenericWindowCanvas* WindowManager::getWindowCanvas(GenericAppWindow* window) const
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
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        // As init might have failed when preparing initial surface
        windowData.second.windowCanvas->reinitResources();
    }
}
