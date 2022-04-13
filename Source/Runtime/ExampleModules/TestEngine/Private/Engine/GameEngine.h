/*!
 * \file GameEngine.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationInstance.h"
#include "Assets/AssetsManager.h"
#include "IRenderInterfaceModule.h"
#include "Editor/Core/ImGui/ImGuiManager.h"

class EngineInputCoreModule;
class IApplicationModule;

struct EngineTime
{
    // Global
    int64 startTick;
    int64 initEndTick;
    uint64 frameCounter = 0;

    // Per frame data
    int64 lastFrameTick;
    int64 frameTick;
    // In Seconds
    // Start with 100FPS
    float lastDeltaTime = 0.01f;
    float deltaTime = 0.01f;

    /* Global time dilation */
    float timeDilation = 1;
    /* Time dilation for activity of app 1 if app is active 0 if app is in background not active*/
    float activeTimeDilation = 1;

    void engineStart();
    void tickStart();
    void progressFrame();
    float getDeltaTime();
};

using EngineEvents = Event<class GameEngine>;

class GameEngine
{
private:
    bool bExitNextFrame = false;

    DelegateHandle renderStateChangeHandle;
    DelegateHandle exitAppHandle;
protected:
    IRenderInterfaceModule* rendererModule;
    IApplicationModule* applicationModule;
    EngineInputCoreModule* inputModule;

    ApplicationInstance* application;
    ImGuiManager imguiManager;
    AssetManager assetManager;
    EngineTime timeData;
private:
    void onRenderStateChange(ERenderStateEvent state);
    void tryExitApp();
protected:
    virtual void onStartUp();
    virtual void onQuit();
    virtual void tickEngine();
public:
    virtual ~GameEngine() = default;

    void startup(const AppInstanceCreateInfo appInstanceCI);
    void engineLoop();
    void quit();

    void requestExit();
    bool isExiting() { return bExitNextFrame; }

    AssetManager& getAssetManager() { return assetManager; }
    ImGuiManager& getImGuiManager() { return imguiManager; }
};

class GameEngineWrapper final
{
private:
    GameEngine* gEngine = nullptr;

    GameEngine* createEngineInstance();

public:

    GameEngineWrapper()
    {
        gEngine = createEngineInstance();
    }

    ~GameEngineWrapper()
    {
        gEngine = nullptr;
    }

    GameEngine* operator->() const
    {
        return gEngine;
    }

    GameEngine* operator*() const
    {
        return gEngine;
    }

    operator bool() const
    {
        return gEngine != nullptr;
    }

    void operator = (const GameEngineWrapper&) = delete;
    void operator = (GameEngineWrapper&&) = delete;
};

inline GameEngineWrapper gEngine;