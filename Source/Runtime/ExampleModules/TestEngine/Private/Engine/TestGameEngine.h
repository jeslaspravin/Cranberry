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
#include "Editor/Core/ImGui/ImGuiManager.h"
#include "IRenderInterfaceModule.h"
#include "Types/Time.h"

class EngineInputCoreModule;
class IApplicationModule;

struct EngineTime
{
    // Global
    TickRep startTick;
    TickRep initEndTick;
    uint64 frameCounter = 0;

    // Per frame data
    TickRep lastFrameTick;
    TickRep frameTick;
    // In Seconds
    // Start with 100FPS
    float lastDeltaTime = 0.01f;
    float deltaTime = 0.01f;

    /* Global time dilation */
    float timeDilation = 1;
    /* Time dilation for activity of app 1 if app is active 0 if app is in background not active*/
    float activeTimeDilation = 1;

    int32 frameRate = 0;
    // -1 means no limit
    TickRep targetFrameTicks = -1;
    // If frame time exceeds required frame time then we skip frames and add excess to slack. Engine loop handles double ticking if needed
    TickRep accumulatedSlack = 0;

    void engineStart();
    void tickStart();
    void progressFrame();
    float getDeltaTime();
    void setTargetFrameRate(uint8 inFameRate)
    {
        accumulatedSlack = 0;
        frameRate = inFameRate;
        if (frameRate == 0)
        {
            targetFrameTicks = -1;
            return;
        }
        targetFrameTicks = Time::fromSeconds(1.0f / float(frameRate));
    }
};

using EngineEvents = Event<class TestGameEngine>;

class TestGameEngine
{
private:
protected:
    IRenderInterfaceModule *rendererModule;
    IApplicationModule *applicationModule;
    EngineInputCoreModule *inputModule;

    ApplicationInstance *application;
    ImGuiManager imguiManager;
    AssetManager assetManager;
    EngineTime timeData;
    bool bActiveWindow = false;
    uint8 frameRateBackup = 0;

protected:
    virtual void onStartUp();
    virtual void onQuit();
    virtual void tickEngine();

public:
    virtual ~TestGameEngine() = default;

    void startup(ApplicationInstance *appInst);
    void engineLoop();
    void quit();

    AssetManager &getAssetManager() { return assetManager; }
    ImGuiManager &getImGuiManager() { return imguiManager; }
};

class GameEngineWrapper final
{
private:
    TestGameEngine *gEngine = nullptr;

    TestGameEngine *createEngineInstance();

public:
    GameEngineWrapper() { gEngine = createEngineInstance(); }

    ~GameEngineWrapper() { gEngine = nullptr; }

    TestGameEngine *operator->() const { return gEngine; }

    TestGameEngine *operator*() const { return gEngine; }

    operator bool() const { return gEngine != nullptr; }

    MAKE_TYPE_NONCOPY_NONMOVE(GameEngineWrapper)
};

inline GameEngineWrapper gEngine;