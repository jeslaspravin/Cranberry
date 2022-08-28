/*!
 * \file World.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "CBEObject.h"
#include "Types/Containers/FlatTree.h"

#ifdef __REF_PARSE__
#include "Classes/Actor.h"
#include "Classes/ActorPrefab.h"
#endif

#include "World.gen.h"

namespace cbe
{
class TransformComponent;
class LogicComponent;
class Actor;
class ActorPrefab;
class World;

struct ComponentWorldTF
{
    TransformComponent *component;
    Transform3D worldTx;
};

namespace EWorldState
{
enum Type
{
    Loading,
    Loaded,
    StartingPlay,
    Playing,
    EndingPlay
};

FORCE_INLINE bool isPlayState(EWorldState::Type state) { return state == StartingPlay || state == Playing || state == EndingPlay; }

}; // namespace EWorldState

using WorldActorEvent = Event<World, Actor *>;
using WorldComponentEvent = Event<World, Object *>;

/**
 * If any struct/logic is changed here in world, Be sure to update the changes in EditorHelpers implementations for world
 */
class ENGINECORE_EXPORT World : public Object
{
    GENERATED_CODES()
public:
    using TFHierarchyIdx = SizeT;

private:
#if EDITOR_BUILD
    // For editor functionalities, check EditorHelpers to modify world in editor
    friend class EditorHelpers;
#endif
    friend class WorldsManager;

    struct META_ANNOTATE() ActorAttachedToInfo
    {
        GENERATED_CODES()

        META_ANNOTATE()
        Actor *actor;

        META_ANNOTATE()
        TransformComponent *component;
    };

    // No need to have as reflected field as removing or adding actors handle this
    using TFHierarchyIdx = SizeT;
    FlatTree<ComponentWorldTF, TFHierarchyIdx> txHierarchy;
    std::unordered_map<TransformComponent *, TFHierarchyIdx> compToTf;

    META_ANNOTATE()
    std::vector<ActorPrefab *> actorPrefabs;
    META_ANNOTATE()
    std::unordered_map<Actor *, ActorAttachedToInfo> actorAttachedTo;

    META_ANNOTATE(Transient)
    std::vector<Actor *> actors;

    // Just to hold references
    META_ANNOTATE(Transient)
    std::set<ActorPrefab *> delayInitPrefabs;

    EWorldState::Type worldState;

public:
    WorldActorEvent onActorAdded;
    WorldActorEvent onActorRemoved;
    WorldComponentEvent onTfCompAdded;
    WorldComponentEvent onTfCompRemoved;
    WorldComponentEvent onLogicCompAdded;
    WorldComponentEvent onLogicCompRemoved;

public:
    World();

    /* Object overrides */
    void onConstructed() override;
    ObjectArchive &serialize(ObjectArchive &ar) override;
    /* Overrides ends */

    void tfAttachmentChanged(TransformComponent *attachingComp, TransformComponent *attachedTo);
    void tfComponentAdded(Actor *actor, TransformComponent *tfComponent);
    void tfComponentRemoved(Actor *actor, TransformComponent *tfComponent);
    void logicComponentAdded(Actor *actor, LogicComponent *logicComp);
    void logicComponentRemoved(Actor *actor, LogicComponent *logicComp);

    // Copies otherWorld into this world Updating any same actors, Creating new actors and deleting non existing actors
    bool copyFrom(World *otherWorld);
    // Merges other world into this world and renames duplicate named actors, bMoveActors true means all actors will be moved to this world
    bool mergeWorld(World *otherWorld, bool bMoveActors);

    // Calling in editor worlds is not efficient
    void getComponentsAttachedTo(std::vector<TransformComponent *> &outAttaches, TransformComponent *component, bool bRecurse = false) const;

    TransformComponent *getActorAttachedToComp(Actor *actor) const;
    Actor *getActorAttachedTo(Actor *actor) const;

    const std::vector<Actor *> &getActors() const { return actors; }

    EWorldState::Type getState() const { return worldState; }

private:
    // The in vector must be arranged from parent to children order
    void updateWorldTf(const std::vector<TFHierarchyIdx> &idxsToUpdate);
    // Initializes all actor prefab and pushes them to actors list and attaches all linked actors
    void prepareForPlay();

    // bDelayedInit for actor created from class to allow setting up the prefab directly with in the world
    Actor *addActor(CBEClass actorClass, const String &actorName, EObjectFlags flags, bool bDelayedInit);
    Actor *addActor(ActorPrefab *inPrefab, const String &name, EObjectFlags flags);
    bool finalizeAddActor(ActorPrefab *prefab);
    Actor *setupActorInternal(ActorPrefab *actorPrefab);
    void removeActor(Actor *actor);

    void broadcastActorAdded(Actor *actor) const { onActorAdded.invoke(actor); }
    void broadcastActorRemoved(Actor *actor) const { onActorRemoved.invoke(actor); }
    void broadcastTfCompAdded(Object *tfComp) const { onTfCompAdded.invoke(tfComp); }
    void broadcastTfCompRemoved(Object *tfComp) const { onTfCompRemoved.invoke(tfComp); }
    void broadcastLogicCompAdded(Object *logicComp) const { onLogicCompAdded.invoke(logicComp); }
    void broadcastLogicCompRemoved(Object *logicComp) const { onLogicCompRemoved.invoke(logicComp); }
} META_ANNOTATE(NoExport);

} // namespace cbe
