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
#include "Types/Delegates/Delegate.h"
#include "CBEObject.h"
#include "Render/EngineRenderTypes.h"

#if __REF_PARSE__
#include "Classes/World.h"
#endif
#include "WorldsManager.gen.h"

class EngineRenderScene;

namespace cbe
{

class World;
class WorldsManager;

struct WorldInfo
{
    GENERATED_CODES()

    ComponentRenderSyncInfo frameSyncInfo;
    SharedPtr<EngineRenderScene> renderScene;
} META_ANNOTATE();

using WorldManagerEvent = Event<WorldsManager, World *, bool>;

class ENGINECORE_EXPORT WorldsManager : public Object
{
    GENERATED_CODES()
public:
    constexpr static const uint32 AllocSlotCount = 2;

private:
    META_ANNOTATE()
    World *mainWorld;

    META_ANNOTATE(Transient)
    World *renderingWorld;

#if EDITOR_BUILD
    META_ANNOTATE(Transient)
    World *editorWorld;
#endif
    META_ANNOTATE(Transient)
    World *playingWorld;

    META_ANNOTATE()
    WorldInfo mainWorldInfo;

    META_ANNOTATE()
    std::unordered_map<World *, WorldInfo> otherWorlds;

    WorldManagerEvent worldUnloadEvent;
    WorldManagerEvent worldInitEvent;

public:
    World *initWorld(World *world, bool bAsMainWorld);
    void unloadWorld(World *world);
    void unloadAllWorlds();

    SharedPtr<EngineRenderScene> getWorldRenderScene(World *world) const;
    World *getMainWorld() const { return mainWorld; }
    World *getPlayingWorld() const { return playingWorld; }
    World *getRenderingWorld() const { return renderingWorld; }
#if EDITOR_BUILD
    World *getEditorWorld() const { return editorWorld; }
#endif

    WorldManagerEvent &onWorldUnloadEvent() { return worldUnloadEvent; }
    WorldManagerEvent &onWorldInitEvent() { return worldInitEvent; }

    void tickWorlds(float deltaTime);

private:
    bool isMainWorld(World *world) const { return mainWorld == world || renderingWorld == world; }
} META_ANNOTATE(NoExport);

} // namespace cbe