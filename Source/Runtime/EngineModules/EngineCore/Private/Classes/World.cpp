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
#include "Property/PropertyHelper.h"
#include "CBEPackage.h"
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
    if (getOuterMost() == nullptr || !PropertyHelper::isChildOf<cbe::Package>(getOuterMost()->getType()))
    {
        debugAssertf(BIT_SET(getFlags(), EObjectFlagBits::ObjFlag_Default), "Outer most of non default world must be a valid package!");
        return;
    }
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

void World::tfAttachmentChanged(TransformComponent *attachingComp, TransformComponent *attachedTo)
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

void World::tfComponentAdded(Actor *actor, TransformComponent *tfComponent)
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
    broadcastTfCompAdded(tfComponent);
}

void World::tfComponentRemoved(Actor *actor, TransformComponent *tfComponent)
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
        compToTf.erase(compWorldTfItr);
    }
    broadcastTfCompRemoved(tfComponent);
}

void World::logicComponentAdded(Actor *actor, LogicComponent *logicComp) { broadcastLogicCompAdded(logicComp); }

void World::logicComponentRemoved(Actor *actor, LogicComponent *logicComp) { broadcastLogicCompRemoved(logicComp); }

bool World::copyFrom(World *otherWorld)
{
    if (EWorldState::isPlayState(getState()) || EWorldState::isPlayState(otherWorld->getState()))
    {
        LOG_ERROR("World", "Cannot copy a playing world to another playing world");
        return false;
    }

    bool bAllCopied = true;
    std::unordered_set<ActorPrefab *> prefabsToRemove(actorPrefabs.cbegin(), actorPrefabs.cend());
    // First create any new actors
    for (ActorPrefab *otherPrefab : otherWorld->actorPrefabs)
    {
        ActorPrefab *thisPrefab = get<ActorPrefab>(ObjectPathHelper::getFullPath(otherPrefab->getName().getChar(), this).getChar());
        if (thisPrefab)
        {
            debugAssert(prefabsToRemove.contains(thisPrefab));
            prefabsToRemove.erase(thisPrefab);
            if (!thisPrefab->copyCompatible(otherPrefab))
            {
                thisPrefab->beginDestroy();
                // No need to erase attachments here as it will be taken care when setting up attachments
                std::erase(actorPrefabs, thisPrefab);
                thisPrefab = nullptr;
            }
        }

        if (thisPrefab == nullptr)
        {
            if (ActorPrefab *parentPrefab = otherPrefab->getParentPrefab())
            {
                thisPrefab = create<ActorPrefab, cbe::ActorPrefab *, const String &>(
                    otherPrefab->getName(), this, otherPrefab->getFlags(), parentPrefab, otherPrefab->getActorTemplate()->getName()
                );
            }
            else
            {
                thisPrefab = create<ActorPrefab, StringID, const String &>(
                    otherPrefab->getName(), this, otherPrefab->getFlags(), otherPrefab->getClass()->name,
                    otherPrefab->getActorTemplate()->getName()
                );
            }
            actorPrefabs.emplace_back(thisPrefab);
        }
    }
    // Now copy each prefabs
    for (ActorPrefab *otherPrefab : otherWorld->actorPrefabs)
    {
        ActorPrefab *thisPrefab = get<ActorPrefab>(ObjectPathHelper::getFullPath(otherPrefab->getName().getChar(), this).getChar());
        debugAssert(thisPrefab);

        bAllCopied = bAllCopied && thisPrefab->copyFrom(otherPrefab);
    }

    // Remove unwanted actors
    std::erase_if(
        actorPrefabs,
        [&prefabsToRemove](ActorPrefab *prefab)
        {
            if (prefabsToRemove.contains(prefab))
            {
                prefab->beginDestroy();
                return true;
            }
            return false;
        }
    );

    actorAttachedTo.clear();
    actorAttachedTo.reserve(otherWorld->actorAttachedTo.size());
    for (const std::pair<Actor *const, ActorAttachedToInfo> &otherAttachmentPair : otherWorld->actorAttachedTo)
    {
        debugAssert(otherAttachmentPair.first && otherAttachmentPair.second.actor && otherAttachmentPair.second.component);

        // Getting actor full path from actor to world relative path
        String fullPath = ObjectPathHelper::getFullPath(ObjectPathHelper::getObjectPath(otherAttachmentPair.first, otherWorld).getChar(), this);
        Actor *thisAttachingActor = get<Actor>(fullPath.getChar());
        // Getting actor full path from actor to world relative path
        fullPath = ObjectPathHelper::getFullPath(ObjectPathHelper::getObjectPath(otherAttachmentPair.second.actor, otherWorld).getChar(), this);
        Actor *thisAttachedActor = get<Actor>(fullPath.getChar());
        // Getting component full path from component to actor relative path
        fullPath = ObjectPathHelper::getFullPath(
            ObjectPathHelper::getObjectPath(otherAttachmentPair.second.component, otherAttachmentPair.second.actor).getChar(), thisAttachedActor
        );
        TransformComponent *thisAttachedComp = get<TransformComponent>(fullPath.getChar());

        debugAssert(thisAttachingActor && thisAttachedActor && thisAttachedComp);

        actorAttachedTo[thisAttachingActor] = ActorAttachedToInfo{ thisAttachedActor, thisAttachedComp };
    }

    markDirty(this);
    return bAllCopied;
}

bool World::mergeWorld(World *otherWorld, bool bMoveActors)
{
    if (EWorldState::isPlayState(getState()) || EWorldState::isPlayState(otherWorld->getState()))
    {
        LOG_ERROR("World", "Cannot merge a playing world to another playing world");
        return false;
    }

    uint64 duplicateCounter = 0;
    auto getUniqPrefabName = [&duplicateCounter, this](const String &otherPrefabName) -> String
    {
        String thisPrefabName = otherPrefabName;
        while (get<ActorPrefab>(ObjectPathHelper::getFullPath(thisPrefabName.getChar(), this).getChar()))
        {
            thisPrefabName = otherPrefabName + String::toString(duplicateCounter++);
        }
        return thisPrefabName;
    };

    if (bMoveActors)
    {
        for (ActorPrefab *otherPrefab : otherWorld->actorPrefabs)
        {
            String newName = getUniqPrefabName(otherPrefab->getName());
            INTERNAL_ObjectCoreAccessors::setOuterAndName(otherPrefab, newName, this, otherPrefab->getType());
        }
        actorPrefabs.insert(actorPrefabs.end(), otherWorld->actorPrefabs.begin(), otherWorld->actorPrefabs.end());
        otherWorld->actorPrefabs.clear();
        actorAttachedTo.merge(std::move(otherWorld->actorAttachedTo));
    }
    else
    {
        std::unordered_map<NameString, String> otherPrefabToNew;
        actorPrefabs.reserve(actorPrefabs.size() + otherWorld->actorPrefabs.size());
        for (ActorPrefab *otherPrefab : otherWorld->actorPrefabs)
        {
            String newName = getUniqPrefabName(otherPrefab->getName());
            otherPrefabToNew[otherPrefab->getName().getChar()] = newName;

            ActorPrefab *thisPrefab;
            if (ActorPrefab *parentPrefab = otherPrefab->getParentPrefab())
            {
                thisPrefab = create<ActorPrefab, cbe::ActorPrefab *, const String &>(
                    otherPrefab->getName(), this, otherPrefab->getFlags(), parentPrefab, otherPrefab->getActorTemplate()->getName()
                );
            }
            else
            {
                thisPrefab = create<ActorPrefab, StringID, const String &>(
                    otherPrefab->getName(), this, otherPrefab->getFlags(), otherPrefab->getClass()->name,
                    otherPrefab->getActorTemplate()->getName()
                );
            }
            thisPrefab->copyFrom(otherPrefab);
            actorPrefabs.emplace_back(thisPrefab);
        }

        actorAttachedTo.reserve(actorAttachedTo.size() + otherWorld->actorAttachedTo.size());
        for (const std::pair<Actor *const, ActorAttachedToInfo> &otherAttachmentPair : otherWorld->actorAttachedTo)
        {
            debugAssert(otherAttachmentPair.first && otherAttachmentPair.second.actor && otherAttachmentPair.second.component);

            ActorPrefab *otherAttachingPrefab
                = ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(otherAttachmentPair.first));
            ActorPrefab *otherAttachedPrefab
                = ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(otherAttachmentPair.second.actor));
            debugAssert(
                otherPrefabToNew.contains(otherAttachingPrefab->getName().getChar())
                && otherPrefabToNew.contains(otherAttachedPrefab->getName().getChar())
            );

            ActorPrefab *thisAttachingPrefab = get<ActorPrefab>(
                ObjectPathHelper::getFullPath(otherPrefabToNew[otherAttachingPrefab->getName().getChar()].getChar(), this).getChar()
            );
            ActorPrefab *thisAttachedPrefab = get<ActorPrefab>(
                ObjectPathHelper::getFullPath(otherPrefabToNew[otherAttachedPrefab->getName().getChar()].getChar(), this).getChar()
            );
            // Getting component full path from component to actor relative path
            String fullPath = ObjectPathHelper::getFullPath(
                ObjectPathHelper::getObjectPath(otherAttachmentPair.second.component, otherAttachedPrefab).getChar(), thisAttachedPrefab
            );
            TransformComponent *thisAttachedComp = get<TransformComponent>(fullPath.getChar());
            debugAssert(thisAttachingPrefab && thisAttachedPrefab && thisAttachedComp);

            actorAttachedTo[thisAttachingPrefab->getActorTemplate()]
                = ActorAttachedToInfo{ thisAttachedPrefab->getActorTemplate(), thisAttachedComp };
        }
    }
}

void World::getComponentsAttachedTo(std::vector<TransformComponent *> &outAttaches, TransformComponent *component, bool bRecurse /*= false*/)
    const
{
    debugAssertf(EWorldState::isPlayState(worldState), "Cannot query components attached to another component when not playing!");

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

TransformComponent *World::getActorAttachedToComp(Actor *actor) const
{
    if (EWorldState::isPlayState(worldState))
    {
        return actor->getRootComponent()->getAttachedTo();
    }
    else
    {
        auto itr = actorAttachedTo.find(actor);
        if (itr != actorAttachedTo.cend())
        {
            return itr->second.component;
        }
    }
    return nullptr;
}

Actor *World::getActorAttachedTo(Actor *actor) const
{
    if (EWorldState::isPlayState(worldState))
    {
        return actor->getActorAttachedTo();
    }
    else
    {
        auto itr = actorAttachedTo.find(actor);
        if (itr != actorAttachedTo.cend())
        {
            return itr->second.actor;
        }
    }
    return nullptr;
}

void World::updateWorldTf(const std::vector<TFHierarchyIdx> &idxsToUpdate)
{
    for (TFHierarchyIdx idx : idxsToUpdate)
    {
        TFHierarchyIdx parentIdx = txHierarchy.getNode(idx).parent;
        if (txHierarchy.isValid(parentIdx))
        {
            txHierarchy[idx].worldTx = txHierarchy[parentIdx].worldTx.transform(txHierarchy[idx].component->relativeTf);
        }
        else
        {
            txHierarchy[idx].worldTx = txHierarchy[idx].component->relativeTf;
        }
    }
}

Actor *World::addActor(CBEClass actorClass, const String &actorName, EObjectFlags flags, bool bDelayedInit)
{
    if (EWorldState::isPlayState(worldState))
    {
        flags |= EObjectFlagBits::ObjFlag_Transient;
    }
    // If modifying how actor gets created then check EditorHelpers::addActorToWorld, World::copyFrom and World::mergeWorld
    ActorPrefab *prefab = create<ActorPrefab, StringID, const String &>(actorName, this, flags, actorClass->name, actorName);
    if (bDelayedInit)
    {
        delayInitPrefabs.insert(prefab);
        return prefab->getActorTemplate();
    }
    actorPrefabs.emplace_back(prefab);
    return setupActorInternal(prefab);
}

Actor *World::addActor(ActorPrefab *inPrefab, const String &name, EObjectFlags flags)
{
    if (EWorldState::isPlayState(worldState))
    {
        flags |= EObjectFlagBits::ObjFlag_Transient;
    }
    // If modifying how actor gets created then check EditorHelpers::addActorToWorld, World::copyFrom and World::mergeWorld
    ActorPrefab *prefab = create<ActorPrefab, ActorPrefab *, const String &>(name, this, flags, inPrefab, name);
    actorPrefabs.emplace_back(prefab);
    return setupActorInternal(prefab);
}

bool World::finalizeAddActor(ActorPrefab *prefab)
{
    debugAssert(prefab->getParentPrefab() == nullptr && delayInitPrefabs.contains(prefab));
    auto prefabItr = delayInitPrefabs.find(prefab);
    if (prefabItr != delayInitPrefabs.end())
    {
        delayInitPrefabs.erase(prefabItr);
        actorPrefabs.emplace_back(prefab);
        setupActorInternal(prefab);
        return true;
    }
    return false;
}

cbe::Actor *World::setupActorInternal(ActorPrefab *actorPrefab)
{
    Actor *actor = actorPrefab->getActorTemplate();
    if (EWorldState::isPlayState(worldState))
    {
        actors.emplace_back(actor);
    }
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
        tfAttachmentChanged(actorTransformComp, actorTransformComp->getAttachedTo());
    }

    // Broadcast add events
    broadcastActorAdded(actorPrefab->getActorTemplate());
    for (TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        // Current assumption: Native tf components gets auto added through createComponent
        if (!actorPrefab->isNativeComponent(actorTransformComp))
        {
            broadcastTfCompAdded(actorTransformComp);
        }
    }
    for (LogicComponent *actorLogicComp : actor->getLogicComponents())
    {
        broadcastLogicCompAdded(actorLogicComp);
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
    actorAttachedTo.erase(actor);
    for (auto actorAttachedToItr = actorAttachedTo.begin(); actorAttachedToItr != actorAttachedTo.end();)
    {
        if (actorAttachedToItr->second.actor == actor)
        {
            Actor *actorToDetach = actorAttachedToItr->first;
            actorAttachedToItr = actorAttachedTo.erase(actorAttachedToItr);
            actorToDetach->detachActor();
        }
        else
        {
            ++actorAttachedToItr;
        }
    }

    // Broadcast removed events
    for (TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        tfComponentRemoved(actor, actorTransformComp);
    }
    for (LogicComponent *actorLogicComp : actor->getLogicComponents())
    {
        logicComponentRemoved(actor, actorLogicComp);
    }
    broadcastActorRemoved(actor);
}

} // namespace cbe