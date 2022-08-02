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
class Actor;
class ActorPrefab;

struct ComponentWorldTF
{
    TransformComponent *component;
    Transform3D worldTx;
};

enum class EWorldState
{
    Loading,
    Loaded,
    StartingPlay,
    Playing,
    EndingPlay
};

class ENGINECORE_EXPORT World : public Object
{
    GENERATED_CODES()
public:
    using TFHierarchyIdx = SizeT;

private:
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

    EWorldState worldState;

public:
    World();

    /* Object overrides */
    void onConstructed() override;
    ObjectArchive &serialize(ObjectArchive &ar) override;
    /* Overrides ends */

    void onAttachmentChanged(TransformComponent *attachingComp, TransformComponent *attachedTo);
    void onComponentAdded(Actor *actor, TransformComponent *tfComponent);
    void onComponentRemoved(Actor *actor, TransformComponent *tfComponent);

    void componentsAttachedTo(std::vector<TransformComponent *> &outAttaches, TransformComponent *component, bool bRecurse = false) const;

private:
    // The in vector must be arranged from parent to children order
    void updateWorldTf(const std::vector<TFHierarchyIdx> &idxsToUpdate);

    Actor *addActor(CBEClass actorClass, const String &actorName, EObjectFlags flags);
    Actor *addActor(ActorPrefab *inPrefab, const String &name, EObjectFlags flags);
    Actor *addActorInternal(ActorPrefab *actorPrefab);
    void removeActor(Actor *actor);
} META_ANNOTATE(NoExport);

} // namespace cbe
