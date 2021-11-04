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

void WindowManager::init()
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
    inputSystem->registerWindow(appMainWindow);

    ENQUEUE_COMMAND_NODEBUG(MainWindowInit,
        {
            ManagerData& data = windowsOpened[appMainWindow];
            data.windowCanvas = new WindowCanvas();
            data.windowCanvas->setWindow(appMainWindow);
            data.windowCanvas->init();
        }
        , this);

}

void WindowManager::destroy()
{
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        GenericWindowCanvas* windowCanvas = windowData.second.windowCanvas;
        ENQUEUE_COMMAND_NODEBUG(MainWindowDestroy,
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
    ENQUEUE_COMMAND(InitWindowCanvas)(
        [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
            {
                // As init might have failed when preparing initial surface
                windowData.second.windowCanvas->reinitResources();
            }
            EngineSettings::surfaceSize.set(Size2D(appMainWindow->windowWidth, appMainWindow->windowHeight));
        });
}

void WindowManager::updateWindowCanvas()
{
    ENQUEUE_COMMAND(UpdateWindowCanvas)(
        [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            cmdList->waitIdle();
            for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
            {
                windowData.second.windowCanvas->reinitResources();
            }
            GlobalBuffers::onSurfaceUpdated();
        });
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
    }
}

void WindowManager::deactivateWindow(GenericAppWindow* window)
{
    if (window == activeWindow)
    {
        activeWindow = nullptr;
        inputSystem->resetStates();
    }
}

bool WindowManager::pollWindows()
{
    for (std::pair<GenericAppWindow* const, ManagerData>& windowData : windowsOpened)
    {
        windowData.first->updateWindow();
    }
    inputSystem->updateInputStates();
    return activeWindow != nullptr;
}

const InputSystem* WindowManager::getInputSystem() const
{
    return inputSystem;
}

void WindowManager::onWindowResize(uint32 width, uint32 height, GenericAppWindow* window)
{
    if(window->windowHeight != height || window->windowWidth != width)
    {
        ENQUEUE_COMMAND_NODEBUG(WindowResize, LAMBDA_BODY
            (
                cmdList->waitIdle();
                window->setWindowSize(width, height, true);
                if(window == appMainWindow)
                {
                    Size2D newSize{ window->windowWidth, window->windowHeight };
                    GlobalBuffers::onSurfaceUpdated();
                    EngineSettings::surfaceSize.set(newSize);
                }
            ), this, window, width, height);
        //gEngine->waitOnRenderApi();
    }
}

void WindowManager::onMouseMoved(uint32 xPos, uint32 yPos, GenericAppWindow* window)
{
    Logger::log("Test", "Mouse abs x : %d, y : %d", xPos, yPos);
}

const InputSystem* GenericAppInstance::inputSystem() const
{
    return appWindowManager.getInputSystem();
}
