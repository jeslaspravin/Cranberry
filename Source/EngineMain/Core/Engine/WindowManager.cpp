#include "WindowManager.h"
#include "../Platform/PlatformInstances.h"
#include "GameEngine.h"
#include "../../RenderInterface/PlatformIndependentHeaders.h"
#include "../Logger/Logger.h"
#include "Config/EngineGlobalConfigs.h"
#include "../Input/InputSystem.h"
#include "../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../RenderApi/GBuffersAndTextures.h"


GenericAppWindow* WindowManager::getMainWindow() const
{
    return appMainWindow;
}

void WindowManager::initMain()
{
    inputSystem = new InputSystem();
    appMainWindow = new PlatformAppWindow();

    appMainWindow->setWindowSize(EngineSettings::screenSize.get().x, EngineSettings::screenSize.get().y, false);
    appMainWindow->setWindowName(::gEngine->getAppName());
    appMainWindow->setWindowMode(EngineSettings::fullscreenMode.get());
    appMainWindow->onWindowActivated.bindObject(this, &WindowManager::activateWindow, appMainWindow);
    appMainWindow->onWindowDeactived.bindObject(this, &WindowManager::deactivateWindow, appMainWindow);
    appMainWindow->onResize.bindObject(this, &WindowManager::onWindowResize, appMainWindow);
    appMainWindow->createWindow(::gEngine->getApplicationInstance());

    ENQUEUE_COMMAND(MainWindowInit,
        {
            ManagerData & data = windowsOpened[appMainWindow];
            data.windowCanvas = new WindowCanvas();
            data.windowCanvas->setWindow(appMainWindow);
            data.windowCanvas->init();
        }
        , this);

}

void WindowManager::destroyMain()
{
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        GenericWindowCanvas* windowCanvas = windowData.second.windowCanvas;
        ENQUEUE_COMMAND(MainWindowDestroy,
            {
                windowCanvas->release();
                delete windowCanvas;
            }
            , windowCanvas);

        windowData.second.windowCanvas = nullptr;

        windowData.first->destroyWindow();
        delete windowData.first;
    }
    appMainWindow = nullptr;
    windowsOpened.clear();
    delete inputSystem;
    inputSystem = nullptr;
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
    ENQUEUE_COMMAND(InitWindowCanvas,
        {
            for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
            {
                // As init might have failed when preparing initial surface
                windowData.second.windowCanvas->reinitResources();
            }
            EngineSettings::surfaceSize.set(Size2D(appMainWindow->windowWidth, appMainWindow->windowHeight));
        }, this);
}

void WindowManager::activateWindow(GenericAppWindow* window)
{
    if (window != activeWindow)
    {
        if (activeWindow != nullptr)
        {
            deactivateWindow(activeWindow);
        }
        activeWindow = window;

        // Capture and reset inputs when window become activated again
        inputSystem->resetToStart();
    }
}

void WindowManager::deactivateWindow(GenericAppWindow* window)
{
    if (window == activeWindow)
    {
        inputSystem->clearInputs();

        activeWindow = nullptr;
    }
}

bool WindowManager::pollWindows()
{
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        windowData.first->updateWindow();
    }
    if (activeWindow != nullptr)
    {
        inputSystem->updateInputStates();
        return true;
    }
    return false;
}

const InputSystem* WindowManager::getInputSystem() const
{
    return inputSystem;
}

void WindowManager::onWindowResize(uint32 width, uint32 height, GenericAppWindow* window)
{
    if(window->windowHeight != height || window->windowWidth != width)
    {
        ENQUEUE_COMMAND(WindowResize, LAMBDA_BODY
            (
                cmdList->waitIdle();
                window->setWindowSize(width, height, true);
                if(window == appMainWindow)
                {
                    Size2D newSize{ window->windowWidth, window->windowHeight };
                    GBuffers::onSurfaceResized(newSize);
                    EngineSettings::surfaceSize.set(newSize);
                }
            ), this, window, width, height);
        //gEngine->waitOnRenderApi();
    }
}

const InputSystem* GenericAppInstance::inputSystem() const
{
    return appWindowManager.getInputSystem();
}
