/*!
 * \file World.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Classes/World.h"
#include "Classes/Actor.h"
#include "Classes/ActorPrefab.h"
#include "Serialization/ObjectSerializationHelpers.h"
#include "CBEObjectHelpers.h"
#include "Components/ComponentBase.h"
#include "Memory/StackAllocator.h"

namespace cbe
{

inline constexpr static const uint32 WORLD_SERIALIZER_VERSION = 0;
inline constexpr static const uint32 WORLD_SERIALIZER_CUTOFF_VERSION = 0;
inline STRINGID_CONSTEXPR static const StringID WORLD_SERIALIZER_CUSTOM_VERSION_ID = STRID("WorldSerializer");

World::World()
{
    if (BIT_SET(getOuterMost()->getFlags(), EObjectFlagBits::ObjFlag_PackageLoadPending))
    {
        worldState = EWorldState::Loading;
    }
    else if (BIT_SET(getOuterMost()->getFlags(), EObjectFlagBits::ObjFlag_PackageLoaded))
    {
        worldState = EWorldState::Loaded;
    }
}

void World::onConstructed()
{
    BaseType::onConstructed();
    if (BIT_SET(getOuterMost()->getFlags(), EObjectFlagBits::ObjFlag_PackageLoaded))
    {
        worldState = EWorldState::Loaded;
    }
}

ObjectArchive &World::serialize(ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        uint32 dataVersion = ar.getCustomVersion(uint32(WORLD_SERIALIZER_CUSTOM_VERSION_ID));
        // This must crash
        fatalAssertf(
            WORLD_SERIALIZER_CUTOFF_VERSION >= dataVersion, "Version of World %u loaded from package %s is outdated, Minimum supported %u!",
            dataVersion, getOuterMost()->getFullPath(), WORLD_SERIALIZER_CUTOFF_VERSION
        );
    }
    else
    {
        ar.setCustomVersion(uint32(WORLD_SERIALIZER_CUSTOM_VERSION_ID), WORLD_SERIALIZER_VERSION);
    }

    ObjectSerializationHelpers::serializeAllFields(this, ar);

    if (ar.isLoading())
    {
        std::erase(actorPrefabs, nullptr);
        actorAttachedTo.erase(nullptr);

        for (auto actorAttachedToItr = actorAttachedTo.begin(); actorAttachedToItr != actorAttachedTo.end();)
        {
            if (actorAttachedToItr->second.actor == nullptr || actorAttachedToItr->second.component == nullptr)
            {
                actorAttachedToItr = actorAttachedTo.erase(actorAttachedToItr);
            }
            else
            {
                ++actorAttachedToItr;
            }
        }
    }
    return ar;
}

void World::onAttachmentChanged(TransformComponent *attachingComp, TransformComponent *attachedTo)
{
    debugAssert(attachingComp);

    auto attachingItr = compToTf.find(attachingComp);
    debugAssert(attachingItr != compToTf.end());
    TFHierarchyIdx attachingIdx = attachingItr->second;
    ActorPrefab *attachingPrefab = ActorPrefab::prefabFromCompTemplate(ActorPrefab::objectTemplateFromObj(attachingComp));

    if (attachedTo)
    {
        auto attachedToItr = compToTf.find(attachedTo);
        debugAssert(attachedToItr != compToTf.end());
        TFHierarchyIdx attachedToIdx = attachedToItr->second;
        ActorPrefab *attachedToPrefab = ActorPrefab::prefabFromCompTemplate(ActorPrefab::objectTemplateFromObj(attachedTo));

        txHierarchy.relinkTo(attachingIdx, attachedToIdx);
        if (attachingPrefab != attachedToPrefab)
        {
            debugAssert(attachingPrefab->getRootComponent() == attachingComp);
            actorAttachedTo[attachingPrefab->getActorTemplate()] = { attachedToPrefab->getActorTemplate(), attachedTo };
        }
    }
    else
    {
        txHierarchy.relinkTo(attachingIdx);
        // Detaching root component so remove actor as well
        if (attachingPrefab->getRootComponent() == attachingComp)
        {
            actorAttachedTo.erase(attachingPrefab->getActorTemplate());
        }
    }

    std::vector<TFHierarchyIdx> idxsToUpdate;
    idxsToUpdate.emplace_back(attachingIdx);
    txHierarchy.getChildren(idxsToUpdate, attachingIdx, true);
    updateWorldTf(idxsToUpdate);
}

void World::onComponentAdded(Actor *actor, TransformComponent *tfComponent)
{
    auto compWorldTfItr = compToTf.find(tfComponent);
    debugAssert(compWorldTfItr == compToTf.end());

    if (tfComponent->getAttachedTo())
    {
        auto parentWorldTfItr = compToTf.find(tfComponent->getAttachedTo());
        debugAssert(parentWorldTfItr != compToTf.end());

        compToTf[tfComponent] = txHierarchy.add(
            ComponentWorldTF{ tfComponent, txHierarchy[parentWorldTfItr->second].worldTx.transform(tfComponent->relativeTf) },
            parentWorldTfItr->second
        );
    }
    else
    {
        compToTf[tfComponent] = txHierarchy.add(ComponentWorldTF{ tfComponent, tfComponent->relativeTf });
    }
}

void World::onComponentRemoved(Actor *actor, TransformComponent *tfComponent)
{
    auto compWorldTfItr = compToTf.find(tfComponent);
    TFHierarchyIdx compTfIdx;
    if (compWorldTfItr != compToTf.end())
    {
        compTfIdx = compWorldTfItr->second;
        std::vector<TFHierarchyIdx> directAttachments;
        txHierarchy.getChildren(directAttachments, compTfIdx, false);

        for (TFHierarchyIdx attachedIdx : directAttachments)
        {
            if (txHierarchy[attachedIdx].component->getActor() == actor)
            {
                txHierarchy[attachedIdx].component->attachComponent(actor->getRootComponent());
            }
            else
            {
                txHierarchy[attachedIdx].component->getActor()->detachActor();
            }
        }
        // By this point all of the attachment of tfComponent will be detached or attached to something else
        txHierarchy.remove(compTfIdx);
    }
}

void World::componentsAttachedTo(std::vector<TransformComponent *> &outAttaches, TransformComponent *component, bool bRecurse /*= false*/) const
{
    auto compWorldTfItr = compToTf.find(component);
    TFHierarchyIdx compTfIdx;
    if (compWorldTfItr != compToTf.end())
    {
        compTfIdx = compWorldTfItr->second;
        std::vector<TFHierarchyIdx> attachments;
        txHierarchy.getChildren(attachments, compTfIdx, bRecurse);
        outAttaches.reserve(attachments.size());

        for (TFHierarchyIdx attachedIdx : attachments)
        {
            outAttaches.emplace_back(txHierarchy[compTfIdx].component);
        }
    }
}

void World::updateWorldTf(const std::vector<TFHierarchyIdx> &idxsToUpdate)
{
    for (TFHierarchyIdx idx : idxsToUpdate)
    {
        TFHierarchyIdx parentIdx = txHierarchy.getNode(idx).parent;
        txHierarchy[idx].worldTx = txHierarchy[parentIdx].worldTx.transform(txHierarchy[idx].component->relativeTf);
    }
}

Actor *World::addActor(CBEClass actorClass, const String &actorName, EObjectFlags flags)
{
    if (worldState == EWorldState::Playing || worldState == EWorldState::StartingPlay || worldState == EWorldState::EndingPlay)
    {
        flags |= EObjectFlagBits::ObjFlag_Transient;
    }
    ActorPrefab *prefab = create<ActorPrefab, StringID, const String &>(actorName, this, flags, actorClass->name, actorName);
    return addActorInternal(prefab);
}

Actor *World::addActor(ActorPrefab *inPrefab, const String &name, EObjectFlags flags)
{
    if (worldState == EWorldState::Playing || worldState == EWorldState::StartingPlay || worldState == EWorldState::EndingPlay)
    {
        flags |= EObjectFlagBits::ObjFlag_Transient;
    }
    ActorPrefab *prefab = create<ActorPrefab, ActorPrefab *, const String &>(name, this, flags, inPrefab, name);
    return addActorInternal(prefab);
}

cbe::Actor *World::addActorInternal(ActorPrefab *actorPrefab)
{
    actorPrefabs.emplace_back(actorPrefab);
    Actor *actor = actorPrefab->getActorTemplate();
    actors.emplace_back(actor);
    ActorPrefab::initializeActor(actorPrefab);
    debugAssert(actor->getRootComponent()->getAttachedTo() == nullptr);

    // Inserting each TransformComponent into global TF tree
    for (TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        compToTf[actorTransformComp] = txHierarchy.add(ComponentWorldTF{ actorTransformComp, actorTransformComp->relativeTf });
    }
    // Updating its attachments, Reason for separated setup is that transformComps will not be ordered from root to leafs
    for (TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        onAttachmentChanged(actorTransformComp, actorTransformComp->getAttachedTo());
    }

    return actorPrefab->getActorTemplate();
}

void World::removeActor(Actor *actor)
{
    ObjectTemplate *actorTemplate = ActorPrefab::objectTemplateFromObj(actor);
    ActorPrefab *prefab = ActorPrefab::prefabFromActorTemplate(actorTemplate);
    if (prefab)
    {
        std::erase(actorPrefabs, prefab);
    }
    std::erase(actors, actor);
}

} // namespace cbe