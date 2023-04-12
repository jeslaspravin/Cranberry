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
    ObjectPrivateDataView worldDatV = world->getObjectData();
    if (bAsMainWorld)
    {
        if (isMainWorld(world))
        {
            return mainWorld;
        }
        unloadWorld(mainWorld);

        LOG("WorldManager", "Initializing main world %.*s", worldDatV.path.length(), worldDatV.path.data());
        mainWorld = world;
        renderingWorld = mainWorld;
        playingWorld = mainWorld;
#if EDITOR_BUILD
        renderingWorld = editorWorld
            = create<World>(worldDatV.name, ICoreObjectsModule::get()->getTransientPackage(), EObjectFlagBits::ObjFlag_Transient);
        editorWorld->copyFrom(mainWorld);
        playingWorld = nullptr;
#endif
        // So that render scene can get the actors immediately and setup initial scene
        renderingWorld->prepareForPlay();
        mainWorldInfo.renderScene = std::make_shared<EngineRenderScene>(renderingWorld);
        onWorldInitEvent().invoke(mainWorld, true);
        return mainWorld;
    }

    if (otherWorlds.contains(world))
    {
        return world;
    }

    LOG("WorldManager", "Initializing world %.*s", worldDatV.path.length(), worldDatV.path.data());
    otherWorlds[world] = { .renderScene = std::make_shared<EngineRenderScene>(world) };
    world->prepareForPlay();
    onWorldInitEvent().invoke(world, false);
    return world;
}

SharedPtr<EngineRenderScene> WorldsManager::getWorldRenderScene(World *world) const
{
    if (isMainWorld(world))
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

    ObjectPrivateDataView worldDatV = world->getObjectData();
    if (isMainWorld(world))
    {
        LOG("WorldManager", "Unloading main world %s", worldDatV.path);
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
    else if (otherWorlds.contains(world))
    {
        LOG("WorldManager", "Unloading world %s", worldDatV.path);
        onWorldUnloadEvent().invoke(world, false);
        world->beginDestroy();
        otherWorlds.erase(world);
    }
}

void WorldsManager::unloadAllWorlds()
{
    if (mainWorld)
    {
        StringView fullPath = mainWorld->getObjectData().path;
        LOG("WorldManager", "Unloading main world %.*s", fullPath.length(), fullPath.data());
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
        StringView fullPath = otherWorld.first->getObjectData().path;
        LOG("WorldManager", "Unloading world %.*s", fullPath.length(), fullPath.data());
        onWorldUnloadEvent().invoke(otherWorld.first, false);
        otherWorld.first->beginDestroy();
    }
    otherWorlds.clear();
}

} // namespace cbe
