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
#include "CBEObjectHelpers.h"
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
        mainWorld = world;
        mainRenderWorld = mainWorld;
#if EDITOR_BUILD
        // TODO(Jeslas) : Create copy of main world for editing
#endif
        // TODO(Jeslas) : Initialize main renderable world to make it suitable for playing or rendering
        mainWorldInfo.renderScene = std::make_shared<EngineRenderScene>(mainRenderWorld);
        onWorldInitEvent().invoke(world, true);
        return mainWorld;
    }

    if (otherWorlds.contains(world))
    {
        return world;
    }

    otherWorlds[world] = { std::make_shared<EngineRenderScene>(world) };
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
        onWorldUnloadEvent().invoke(world, true);
        unloadWorldInternal(world);
        mainWorldInfo.renderScene.reset();
    }
    else if (otherWorlds.contains(world))
    {
        onWorldUnloadEvent().invoke(world, false);
        unloadWorldInternal(world);
        otherWorlds.erase(world);
    }
}

void WorldsManager::unloadAllWorlds()
{
    if (mainWorld)
    {
        unloadWorldInternal(mainWorld);
        mainWorldInfo.renderScene.reset();
    }
    for (const std::pair<World *const, WorldInfo> &otherWorld : otherWorlds)
    {
        unloadWorldInternal(otherWorld.first);
    }
    otherWorlds.clear();
}

void WorldsManager::unloadWorldInternal(World *world) { world->beginDestroy(); }

} // namespace cbe
