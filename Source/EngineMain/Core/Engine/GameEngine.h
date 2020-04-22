#pragma once

#include "../Memory/SmartPointers.h"
#include "../../RenderApi/RenderApi.h"
#include "../Platform/GenericAppInstance.h"

struct EngineTime
{
    int64 startTick;
    int64 initEndTick;
    int64 lastFrameTick;
    int64 frameTick;
    // In Seconds
    float lastDeltaTime = 0;
    float deltaTime = 0;

    float timeDilation = 1;

    void engineStart();
    void tickStart();
    void progressTick();
    float getDeltaTime();
};

class GameEngine
{
private:
    GenericAppInstance* applicationInstance;
    bool bExitNextFrame = false;
protected:
    UniquePtr<RenderApi> renderingApi;
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
    const RenderApi* getRenderApi() const { return renderingApi.get(); }

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
        delete gEngine;
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