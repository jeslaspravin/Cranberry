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

public:

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

inline GameEngine* gEngine = nullptr;