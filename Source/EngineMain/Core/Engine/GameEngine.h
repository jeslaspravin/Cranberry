#pragma once

#include "../Memory/SmartPointers.h"
#include "../../RenderApi/RenderApi.h"
#include "../Platform/GenericAppInstance.h"
class GameEngine
{
private:
    GenericAppInstance* applicationInstance;
    bool bExitNextFrame = false;
protected:
    UniquePtr<RenderApi> renderingApi;


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