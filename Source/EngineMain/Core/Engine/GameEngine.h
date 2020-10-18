#pragma once

#include "../../RenderApi/RenderApi.h"
#include "../Platform/GenericAppInstance.h"

struct EngineTime
{
    uint64 frameCounter = 0;
    int64 startTick;
    int64 initEndTick;
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

class GameEngine
{
private:
    GenericAppInstance* applicationInstance;
    bool bExitNextFrame = false;
protected:
    RenderApi renderingApi;
    EngineTime timeData;

    virtual void onStartUp();
    virtual void onQuit();
    virtual void tickEngine();

public:
    virtual ~GameEngine() = default;

    void startup(GenericAppInstance* appInstance);
    void engineLoop();
    void quit();

    void requestExit();
    bool isExiting() { return bExitNextFrame; }

    const String& getAppName() const;
    void getVersion(int32& head, int32& major, int32& sub) const;
    const GenericAppInstance* getApplicationInstance() const;
    GenericAppInstance& appInstance() const;

    const RenderApi* getRenderApi() const { return &renderingApi; }
    void issueRenderCommand(class IRenderCommand* renderCommand);
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