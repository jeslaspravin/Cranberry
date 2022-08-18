/*!
 * \file WorldsManager.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "CBEObject.h"

#if __REF_PARSE__
#include "Classes/World.h"
#endif
#include "WorldsManager.gen.h"

class EngineRenderScene;

namespace cbe
{

class World;
class WorldsManager;

struct META_ANNOTATE() WorldInfo
{
    GENERATED_CODES()

    SharedPtr<EngineRenderScene> renderScene;
};

using WorldManagerEvent = Event<WorldsManager, World *, bool>;

class ENGINECORE_EXPORT WorldsManager : public Object
{
    GENERATED_CODES()
public:
    constexpr static const uint32 AllocSlotCount = 2;

private:
    META_ANNOTATE()
    World *mainWorld;
    META_ANNOTATE()
    WorldInfo mainWorldInfo;

    META_ANNOTATE()
    std::unordered_map<World *, WorldInfo> otherWorlds;

    WorldManagerEvent worldUnloadEvent;
    WorldManagerEvent worldInitEvent;

public:
    World *initWorld(World *world, bool bAsMainWorld);
    SharedPtr<EngineRenderScene> getWorldRenderScene(World *world) const;

    void unloadWorld(World *world);
    void unloadAllWorlds();

    WorldManagerEvent &onWorldUnloadEvent() { return worldUnloadEvent; }
    WorldManagerEvent &onWorldInitEvent() { return worldInitEvent; }

private:
    FORCE_INLINE void unloadWorldInternal(World *world);
} META_ANNOTATE(NoExport);

} // namespace cbe