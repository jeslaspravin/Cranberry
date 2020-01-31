#include "GameEngine.h"
#include "../../RenderApi/RenderApi.h"
#include "../Logger/Logger.h"
#include <assert.h>

GameEngine* gEngine = nullptr;

void GameEngine::startup(GenericAppInstance* appInstance)
{	
	applicationInstance = appInstance;
	renderingApi=UniquePtr<RenderApi>(new RenderApi());
	renderingApi->initialize();
	onStartUp();
}

void GameEngine::onStartUp()
{

}

void GameEngine::quit()
{
	onQuit();
	renderingApi->destroy();
	renderingApi.release();
	applicationInstance = nullptr;
}

const String& GameEngine::getAppName() const
{
	assert(applicationInstance);
	return applicationInstance->applicationName;
}

void GameEngine::getVersion(int32& head, int32& major, int32& sub) const
{
	assert(applicationInstance);
	head = applicationInstance->headVersion;
	major = applicationInstance->majorVersion;
	sub = applicationInstance->subVersion;
}

const GenericAppInstance* GameEngine::getApplicationInstance() const
{
	return applicationInstance;
}

void GameEngine::onQuit()
{
	
}

