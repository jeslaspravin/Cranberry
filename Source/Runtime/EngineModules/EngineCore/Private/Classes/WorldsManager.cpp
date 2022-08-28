/*!
 * \file WorldsManager.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Classes/WorldsManager.h"
#include "ICoreObjectsModule.h"
#include "CBEObjectHelpers.h"
#include "CBEPackage.h"
#include "Classes/World.h"
#include "EngineRenderScene.h"

namespace cbe
{

World *WorldsManager::initWorld(World *world, bool bAsMainWorld)
{
    if (bAsMainWorld)
    {
        if (world == mainWorld)
        {
            return mainWorld;
        }
        unloadWorld(mainWorld);

        LOG("WorldManager", "Initializing main world %s", world->getFullPath());
        mainWorld = world;
        renderingWorld = mainWorld;
        playingWorld = mainWorld;
#if EDITOR_BUILD
        renderingWorld = editorWorld
            = create<World>(mainWorld->getName(), ICoreObjectsModule::get()->getTransientPackage(), EObjectFlagBits::ObjFlag_Transient);
        editorWorld->copyFrom(mainWorld);
        playingWorld = nullptr;
#endif
        // So that render scene can get the actors immediately and setup initial scene
        renderingWorld->prepareForPlay();
        mainWorldInfo.renderScene = std::make_shared<EngineRenderScene>(renderingWorld);
        onWorldInitEvent().invoke(world, true);
        return mainWorld;
    }

    if (otherWorlds.contains(world))
    {
        return world;
    }

    LOG("WorldManager", "Initializing world %s", world->getFullPath());
    otherWorlds[world] = { std::make_shared<EngineRenderScene>(world) };
    world->prepareForPlay();
    onWorldInitEvent().invoke(world, false);
    return world;
}

SharedPtr<EngineRenderScene> WorldsManager::getWorldRenderScene(World *world) const
{
    if (mainWorld == world)
    {
        return mainWorldInfo.renderScene;
    }

    auto otherWorldItr = otherWorlds.find(world);
    if (otherWorldItr != otherWorlds.cend())
    {
        return otherWorldItr->second.renderScene;
    }
    return nullptr;
}

void WorldsManager::unloadWorld(World *world)
{
    if (!isValid(world))
    {
        return;
    }

    if (mainWorld == world)
    {
        LOG("WorldManager", "Unloading main world %s", world->getFullPath());
        onWorldUnloadEvent().invoke(world, true);
#if EDITOR_BUILD
        editorWorld->beginDestroy();
        if (playingWorld)
        {
            playingWorld->beginDestroy();
        }
#endif
        mainWorld = nullptr;
        renderingWorld = nullptr;
        playingWorld = nullptr;
        mainWorldInfo.renderScene.reset();
    }
    else if (otherWorlds.contains(world))
    {
        LOG("WorldManager", "Unloading world %s", world->getFullPath());
        onWorldUnloadEvent().invoke(world, false);
        world->beginDestroy();
        otherWorlds.erase(world);
    }
}

void WorldsManager::unloadAllWorlds()
{
    if (mainWorld)
    {
        LOG("WorldManager", "Unloading main world %s", mainWorld->getFullPath());
        onWorldUnloadEvent().invoke(mainWorld, true);
#if EDITOR_BUILD
        editorWorld->beginDestroy();
        if (playingWorld)
        {
            playingWorld->beginDestroy();
        }
#endif
        mainWorld = nullptr;
        renderingWorld = nullptr;
        playingWorld = nullptr;
        mainWorldInfo.renderScene.reset();
    }
    for (const std::pair<World *const, WorldInfo> &otherWorld : otherWorlds)
    {
        LOG("WorldManager", "Unloading world %s", otherWorld.first->getFullPath());
        onWorldUnloadEvent().invoke(otherWorld.first, false);
        otherWorld.first->beginDestroy();
    }
    otherWorlds.clear();
}

} // namespace cbe
