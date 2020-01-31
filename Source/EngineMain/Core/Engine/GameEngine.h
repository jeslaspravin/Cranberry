#pragma once

#include "../Memory/SmartPointers.h"
#include "../../RenderApi/RenderApi.h"
#include "../Platform/GenericAppInstance.h"
class GameEngine
{
private:
	GenericAppInstance* applicationInstance;
protected:
	UniquePtr<RenderApi> renderingApi;


	virtual void onStartUp();
	virtual void onQuit();

public:

	void startup(GenericAppInstance* appInstance);

	void quit();

	const String& getAppName() const;
	void getVersion(int32& head, int32& major, int32& sub) const;
	const GenericAppInstance* getApplicationInstance() const;
};

extern GameEngine* gEngine;