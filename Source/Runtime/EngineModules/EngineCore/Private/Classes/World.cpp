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
#include "ObjectTemplate.h"
#include "Property/PropertyHelper.h"
#include "CBEPackage.h"
#include "WACHelpers.h"
#include "Classes/Actor.h"
#include "Classes/ActorPrefab.h"
#include "Serialization/ObjectSerializationHelpers.h"
#include "CBEObjectHelpers.h"
#include "Components/ComponentBase.h"
#include "Memory/StackAllocator.h"

namespace cbe
{

constexpr inline const uint32 WORLD_SERIALIZER_VERSION = 0;
constexpr inline const uint32 WORLD_SERIALIZER_CUTOFF_VERSION = 0;
STRINGID_CONSTEXPR inline const StringID WORLD_SERIALIZER_CUSTOM_VERSION_ID = STRID("WorldSerializer");

World::World()
{
    Object *package = getOuterMost();
    if (package == nullptr || !PropertyHelper::isChildOf<cbe::Package>(package->getType()))
    {
        debugAssertf(
            BIT_SET(getObjectData().flags, EObjectFlagBits::ObjFlag_Default), "Outer most of non default world must be a valid package!"
        );
        return;
    }

    ObjectPrivateDataView packageDatV = package->getObjectData();
    if (BIT_SET(packageDatV.flags, EObjectFlagBits::ObjFlag_PackageLoadPending))
    {
        worldState = EWorldState::Loading;
    }
    else if (BIT_SET(packageDatV.flags, EObjectFlagBits::ObjFlag_PackageLoaded))
    {
        worldState = EWorldState::Loaded;
    }
}

void World::onConstructed()
{
    BaseType::onConstructed();
    if (BIT_SET(getOuterMost()->getObjectData().flags, EObjectFlagBits::ObjFlag_PackageLoaded))
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
            WORLD_SERIALIZER_CUTOFF_VERSION >= dataVersion, "Version of World {} loaded from package {} is outdated, Minimum supported {}!",
            dataVersion, getObjectData().path, WORLD_SERIALIZER_CUTOFF_VERSION
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

void World::tfCompTransformed(TransformComponent *tfComponent)
{
    debugAssert(EWorldState::isPreparedState(getState()));

    std::vector<TransformComponent *> transformedComps;
    std::vector<TransformLeafComponent *> transformedLeaves;

    auto attachedToItr = compToTf.find(tfComponent);
    if (attachedToItr != compToTf.cend())
    {
        TFHierarchyIdx attachedToIdx = attachedToItr->second;
        debugAssert(txHierarchy.isValid(attachedToIdx));

        std::vector<TFHierarchyIdx> idxsToUpdate;
        idxsToUpdate.emplace_back(attachedToIdx);
        txHierarchy.getChildren(idxsToUpdate, attachedToIdx, true);
        updateWorldTf(idxsToUpdate);

        // Add the components to broadcast event
        transformedComps.reserve(idxsToUpdate.size());
        for (TFHierarchyIdx tfIdx : idxsToUpdate)
        {
            TransformComponent *tfComp = txHierarchy[tfIdx].component;
            WACHelpers::getComponentLeafs(tfComp, transformedLeaves);
            transformedComps.emplace_back(tfComp);
        }
    }

    // Broadcast events
    std::erase_if(
        transformedComps,
        [this](TransformComponent *comp)
        {
            return !dirtyTfComps.insert(comp).second;
        }
    );
    std::erase_if(
        transformedLeaves,
        [this](TransformLeafComponent *comp)
        {
            return !dirtyLeafComps.insert(comp).second;
        }
    );
    if (!transformedComps.empty())
    {
        broadcastTfCompTransformed(transformedComps);
    }
    if (!transformedLeaves.empty())
    {
        broadcastLeafTransformed(transformedLeaves);
    }
}

void World::tfAttachmentChanged(TransformComponent *attachingComp, TransformComponent *attachedTo)
{
    debugAssert(EWorldState::isPreparedState(getState()));
    debugAssert(attachingComp);

    Actor *attachingActor = attachingComp->getActor();
    if (attachedTo)
    {
        Actor *attachedToActor = attachedTo->getActor();
        if (attachingActor != attachedToActor)
        {
            debugAssert(attachingActor->getRootComponent() == attachingComp);
            actorAttachedTo[attachingActor] = { attachedToActor, attachedTo };
        }
    }
    else
    {
        // Detaching root component so remove actor as well
        if (attachingActor->getRootComponent() == attachingComp)
        {
            actorAttachedTo.erase(attachingActor);
        }
    }

    updateTfAttachment(attachingComp, attachedTo, true);
}

void World::tfComponentAdded(Actor * /*actor*/, TransformComponent *tfComponent)
{
    debugAssert(EWorldState::isPreparedState(getState()));

    auto compWorldTfItr = compToTf.find(tfComponent);
    debugAssert(compWorldTfItr == compToTf.end());

    if (TransformComponent *attachedToTf = tfComponent->getAttachedTo())
    {
        auto parentWorldTfItr = compToTf.find(attachedToTf);
        debugAssert(parentWorldTfItr != compToTf.end());

        compToTf[tfComponent] = txHierarchy.add(
            ComponentWorldTF{ tfComponent, txHierarchy[parentWorldTfItr->second].worldTx.transform(tfComponent->getRelativeTransform()) },
            parentWorldTfItr->second
        );
    }
    else
    {
        compToTf[tfComponent] = txHierarchy.add(ComponentWorldTF{ tfComponent, tfComponent->getRelativeTransform() });
    }
    broadcastTfCompAdded(tfComponent);
}

void World::tfComponentRemoved(Actor *actor, TransformComponent *tfComponent)
{
    debugAssert(EWorldState::isPreparedState(getState()));

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
                WACHelpers::attachComponent(txHierarchy[attachedIdx].component, actor->getRootComponent());
            }
            else
            {
                WACHelpers::detachActor(txHierarchy[attachedIdx].component->getActor());
            }
        }
        // By this point all of the attachment of tfComponent will be detached or attached to something else
        txHierarchy.remove(compTfIdx);
        compToTf.erase(compWorldTfItr);
    }
    broadcastTfCompRemoved(tfComponent);
}

void World::leafComponentAdded(Actor * /*actor*/, TransformLeafComponent *leafComp)
{
    debugAssert(EWorldState::isPreparedState(getState()));
    broadcastLeafCompAdded(leafComp);
}

void World::leafComponentRemoved(Actor * /*actor*/, TransformLeafComponent *leafComp)
{
    debugAssert(EWorldState::isPreparedState(getState()));
    broadcastLeafCompRemoved(leafComp);
}

void World::logicComponentAdded(Actor * /*actor*/, LogicComponent *logicComp)
{
    debugAssert(EWorldState::isPreparedState(getState()));
    broadcastLogicCompAdded(logicComp);
}

void World::logicComponentRemoved(Actor * /*actor*/, LogicComponent *logicComp)
{
    debugAssert(EWorldState::isPreparedState(getState()));
    broadcastLogicCompRemoved(logicComp);
}

bool World::copyFrom(World *otherWorld)
{
    if (EWorldState::isPlayState(getState()) || EWorldState::isPlayState(otherWorld->getState()))
    {
        LOG_ERROR("World", "Cannot copy a playing world to another playing world");
        return false;
    }
    const CoreObjectsDB &objsDb = ICoreObjectsModule::objectsDB();

    bool bAllCopied = true;
    std::unordered_set<ActorPrefab *> prefabsToRemove(actorPrefabs.cbegin(), actorPrefabs.cend());
    // First create any new actors
    for (ActorPrefab *otherPrefab : otherWorld->actorPrefabs)
    {
        ObjectPrivateDataView otherPrefabDatV = objsDb.getObjectData(otherPrefab->getDbIdx());
        ActorPrefab *thisPrefab = get<ActorPrefab>(ObjectPathHelper::getFullPath(otherPrefabDatV.name, this));
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
                thisPrefab = create<ActorPrefab, cbe::ActorPrefab *, String>(
                    otherPrefabDatV.name, this, otherPrefabDatV.flags, parentPrefab, otherPrefab->getActorTemplate()->getObjectData().name
                );
            }
            else
            {
                thisPrefab = create<ActorPrefab, StringID, String>(
                    otherPrefabDatV.name, this, otherPrefabDatV.flags, otherPrefab->getActorClass()->name,
                    otherPrefab->getActorTemplate()->getObjectData().name
                );
            }
            actorPrefabs.emplace_back(thisPrefab);
        }
    }
    // Now copy each prefabs
    for (ActorPrefab *otherPrefab : otherWorld->actorPrefabs)
    {
        ActorPrefab *thisPrefab = get<ActorPrefab>(ObjectPathHelper::getFullPath(otherPrefab->getObjectData().name, this));
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
        String fullPath = ObjectPathHelper::getFullPath(ObjectPathHelper::computeObjectPath(otherAttachmentPair.first, otherWorld), this);
        Actor *thisAttachingActor = get<Actor>(fullPath);
        // Getting actor full path from actor to world relative path
        fullPath = ObjectPathHelper::getFullPath(ObjectPathHelper::computeObjectPath(otherAttachmentPair.second.actor, otherWorld), this);
        Actor *thisAttachedActor = get<Actor>(fullPath);
        // Getting component full path from component to actor template relative path
        fullPath = ObjectPathHelper::computeObjectPath(
            otherAttachmentPair.second.component, ActorPrefab::objectTemplateFromObj(otherAttachmentPair.second.actor)
        );
        fullPath = ObjectPathHelper::getFullPath(fullPath, ActorPrefab::objectTemplateFromObj(thisAttachedActor));
        TransformComponent *thisAttachedComp = get<TransformComponent>(fullPath);

        debugAssert(thisAttachingActor && thisAttachedActor && thisAttachedComp);

        actorAttachedTo[thisAttachingActor] = ActorAttachedToInfo{ thisAttachedActor, thisAttachedComp };
    }

    /**
     * Replacements are necessary in case there is actor/component references across tree, Then ActorPrefab.copyFrom is alone not enough
     * Example ObjectTemplate.copyFrom if it stores object from another sibling ObjectTemplate then it never gets replaced. It gets handled here
     */
    std::unordered_map<Object *, Object *> replacements = {
        {otherWorld, this}
    };
    std::vector<Object *> objectsToReplace;
    {
        std::vector<Object *> otherSubObjs;
        objsDb.getSubobjects(otherSubObjs, otherWorld->getDbIdx());

        objectsToReplace.reserve(otherSubObjs.size());
        for (Object *otherObj : otherSubObjs)
        {
            String fullPath = ObjectPathHelper::getFullPath(ObjectPathHelper::computeObjectPath(otherObj, otherWorld), this);
            Object *thisObj = get(fullPath);
            debugAssert(thisObj);
            replacements[otherObj] = thisObj;
            objectsToReplace.emplace_back(thisObj);
        }
    }
    // This world is not included in objectsToReplace
    // Could be parallelized
    for (Object *thisObj : objectsToReplace)
    {
        cbe::replaceObjectReferences(thisObj, replacements, EObjectTraversalMode::OnlyObject);
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
    const CoreObjectsDB &objsDb = ICoreObjectsModule::objectsDB();

    uint64 duplicateCounter = 0;
    auto getUniqPrefabName = [&duplicateCounter, this](String otherPrefabName) -> String
    {
        String thisPrefabName = otherPrefabName;
        while (get<ActorPrefab>(ObjectPathHelper::getFullPath(thisPrefabName, this)))
        {
            thisPrefabName = otherPrefabName + String::toString(duplicateCounter++);
        }
        return thisPrefabName;
    };

    if (bMoveActors)
    {
        for (ActorPrefab *otherPrefab : otherWorld->actorPrefabs)
        {
            ObjectPrivateDataView otherPrefabDatV = objsDb.getObjectData(otherPrefab->getDbIdx());

            String newName = getUniqPrefabName(otherPrefabDatV.name);
            INTERNAL_ObjectCoreAccessors::setOuterAndName(otherPrefab, newName, this, otherPrefabDatV.clazz);
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
            ObjectPrivateDataView otherPrefabDatV = objsDb.getObjectData(otherPrefab->getDbIdx());

            String newName = getUniqPrefabName(otherPrefabDatV.name);
            otherPrefabToNew[NameString(otherPrefabDatV.name)] = newName;

            ActorPrefab *thisPrefab;
            if (ActorPrefab *parentPrefab = otherPrefab->getParentPrefab())
            {
                thisPrefab = create<ActorPrefab, cbe::ActorPrefab *, String>(
                    otherPrefabDatV.name, this, otherPrefabDatV.flags, parentPrefab, otherPrefab->getActorTemplate()->getObjectData().name
                );
            }
            else
            {
                thisPrefab = create<ActorPrefab, StringID, String>(
                    otherPrefabDatV.name, this, otherPrefabDatV.flags, otherPrefab->getActorClass()->name,
                    otherPrefab->getActorTemplate()->getObjectData().name
                );
            }
            thisPrefab->copyFrom(otherPrefab);
            actorPrefabs.emplace_back(thisPrefab);
        }

        actorAttachedTo.reserve(actorAttachedTo.size() + otherWorld->actorAttachedTo.size());
        for (const std::pair<Actor *const, ActorAttachedToInfo> &otherAttachmentPair : otherWorld->actorAttachedTo)
        {
            debugAssert(otherAttachmentPair.first && otherAttachmentPair.second.actor && otherAttachmentPair.second.component);

            ActorPrefab *otherAttPrefab = ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(otherAttachmentPair.first));
            ActorPrefab *otherAttToPrefab
                = ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(otherAttachmentPair.second.actor));
            NameString attachedPrefabName{ objsDb.getObjectData(otherAttPrefab->getDbIdx()).name };
            NameString attachToPrefabName{ objsDb.getObjectData(otherAttToPrefab->getDbIdx()).name };
            debugAssert(otherPrefabToNew.contains(attachedPrefabName) && otherPrefabToNew.contains(attachToPrefabName));

            ActorPrefab *thisAttPrefab = get<ActorPrefab>(ObjectPathHelper::getFullPath(otherPrefabToNew[attachedPrefabName], this));
            ActorPrefab *thisAttToPrefab = get<ActorPrefab>(ObjectPathHelper::getFullPath(otherPrefabToNew[attachToPrefabName], this));
            // Getting component full path from component to actor relative path
            String fullPath = ObjectPathHelper::getFullPath(
                ObjectPathHelper::computeObjectPath(otherAttachmentPair.second.component, otherAttToPrefab), thisAttToPrefab
            );
            TransformComponent *thisAttachedComp = get<TransformComponent>(fullPath);
            debugAssert(thisAttPrefab && thisAttToPrefab && thisAttachedComp);

            actorAttachedTo[thisAttPrefab->getActorTemplate()] = ActorAttachedToInfo{ thisAttToPrefab->getActorTemplate(), thisAttachedComp };
        }
    }
    return true;
}

bool World::hasWorldTf(const TransformComponent *component) const
{
    debugAssert(EWorldState::isPreparedState(worldState));

    auto compWorldTfItr = compToTf.find(component);
    if (compWorldTfItr != compToTf.end())
    {
        return txHierarchy.isValid(compWorldTfItr->second);
    }
    return false;
}

const Transform3D &World::getWorldTf(const TransformComponent *component) const
{
    debugAssert(EWorldState::isPreparedState(worldState));

    auto compWorldTfItr = compToTf.find(component);
    if (compWorldTfItr != compToTf.end())
    {
        return txHierarchy[compWorldTfItr->second].worldTx;
    }
    return component->getRelativeTransform();
}

TransformComponent *World::getComponentAttachedTo(const TransformComponent *component) const
{
    debugAssert(EWorldState::isPreparedState(worldState));

    auto compWorldTfItr = compToTf.find(component);
    debugAssert(compWorldTfItr != compToTf.cend());
    TFHierarchyIdx parentIdx = txHierarchy.getNode(compWorldTfItr->second).parent;
    if (txHierarchy.isValid(parentIdx))
    {
        return txHierarchy[parentIdx].component;
    }

    return nullptr;
}

void World::getComponentAttaches(const TransformComponent *component, std::vector<TransformComponent *> &childTfs) const
{
    debugAssert(EWorldState::isPreparedState(worldState));

    auto compWorldTfItr = compToTf.find(component);
    debugAssert(compWorldTfItr != compToTf.cend());

    std::vector<TFHierarchyIdx> directAttachments;
    txHierarchy.getChildren(directAttachments, compWorldTfItr->second, false);
    childTfs.reserve(childTfs.size() + directAttachments.size());
    for (TFHierarchyIdx attachedIdx : directAttachments)
    {
        childTfs.push_back(txHierarchy[attachedIdx].component);
    }
}

TransformComponent *World::getActorAttachedToComp(const Actor *actor) const
{
    debugAssert(EWorldState::isPreparedState(worldState));

    auto compWorldTfItr = compToTf.find(actor->getRootComponent());
    debugAssert(compWorldTfItr != compToTf.end());

    World::TFHierarchyIdx parentIdx = txHierarchy.getNode(compWorldTfItr->second).parent;
    if (txHierarchy.isValid(parentIdx))
    {
        return txHierarchy[parentIdx].component;
    }
    return nullptr;
}

Actor *World::getActorAttachedTo(const Actor *actor) const
{
    debugAssert(EWorldState::isPreparedState(worldState));
    auto compWorldTfItr = compToTf.find(actor->getRootComponent());
    debugAssert(compWorldTfItr != compToTf.end());
    TFHierarchyIdx parentIdx = txHierarchy.getNode(compWorldTfItr->second).parent;
    if (txHierarchy.isValid(parentIdx))
    {
        return txHierarchy[parentIdx].component->getActor();
    }

    return nullptr;
}

Actor *World::spawnActor(ActorPrefab *actorPrefab, Transform3D transform, StringView actorName, bool bDelayedInit)
{
    debugAssert(EWorldState::isPlayState(worldState));

    Actor *actor = addActor(actorPrefab, actorName, EObjectFlagBits::ObjFlag_Transient, true);
    actor->setWorldTransform(transform);
    if (!bDelayedInit)
    {
        finalizeAddActor(ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(actor)));
    }
    return actor;
}

Actor *World::spawnActor(CBEClass actorClass, Transform3D transform, StringView actorName, bool bDelayedInit)
{
    debugAssert(EWorldState::isPlayState(worldState));

    Actor *actor = addActor(actorClass, actorName, EObjectFlagBits::ObjFlag_Transient, true);
    actor->setWorldTransform(transform);
    if (!bDelayedInit)
    {
        finalizeAddActor(ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(actor)));
    }
    return actor;
}

void World::finishSpawning(Actor *actor)
{
    debugAssert(EWorldState::isPlayState(worldState));
    finalizeAddActor(ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(actor)));
}

void World::updateWorldTf(const std::vector<TFHierarchyIdx> &idxsToUpdate)
{
    for (TFHierarchyIdx idx : idxsToUpdate)
    {
        TFHierarchyIdx parentIdx = txHierarchy.getNode(idx).parent;
        if (txHierarchy.isValid(parentIdx))
        {
            txHierarchy[idx].worldTx = txHierarchy[parentIdx].worldTx.transform(txHierarchy[idx].component->getRelativeTransform());
        }
        else
        {
            txHierarchy[idx].worldTx = txHierarchy[idx].component->getRelativeTransform();
        }
    }
}

void World::prepareForPlay()
{
    debugAssert(!EWorldState::isPreparedState(worldState));

    actors.clear();
    actors.reserve(actorPrefabs.size());
    for (ActorPrefab *prefab : actorPrefabs)
    {
        setupActorInternal(prefab, false);
        actors.emplace_back(prefab->getActorTemplate());
    }

    for (Actor *actor : getActors())
    {
        if (actorAttachedTo.contains(actor))
        {
            WACHelpers::attachActor(actor, actorAttachedTo[actor].component);
        }
    }

    worldState = EWorldState::PreparedPlay;
}

void World::commitDirtyComponents()
{
    dirtyLeafComps.clear();
    dirtyTfComps.clear();
}

Actor *World::addActor(CBEClass actorClass, StringView actorName, EObjectFlags actorFlags, bool bDelayedInit)
{
    if (EWorldState::isPlayState(worldState))
    {
        actorFlags |= EObjectFlagBits::ObjFlag_Transient;
    }
    // If modifying how actor gets created then check EditorHelpers::addActorToWorld, World::copyFrom and World::mergeWorld
    ActorPrefab *prefab = create<ActorPrefab, StringID, String>(actorName, this, actorFlags, actorClass->name, actorName);
    if (bDelayedInit)
    {
        delayInitPrefabs.insert(prefab);
        return prefab->getActorTemplate();
    }
    actorPrefabs.emplace_back(prefab);
    return setupActorInternal(prefab, false);
}

Actor *World::addActor(ActorPrefab *inPrefab, StringView name, EObjectFlags actorFlags, bool bDelayedInit)
{
    if (EWorldState::isPlayState(worldState))
    {
        actorFlags |= EObjectFlagBits::ObjFlag_Transient;
    }
    // If modifying how actor gets created then check EditorHelpers::addActorToWorld, World::copyFrom and World::mergeWorld
    ActorPrefab *prefab = create<ActorPrefab, ActorPrefab *, String>(name, this, actorFlags, inPrefab, name);
    if (bDelayedInit)
    {
        delayInitPrefabs.insert(prefab);
        return prefab->getActorTemplate();
    }
    actorPrefabs.emplace_back(prefab);
    return setupActorInternal(prefab, false);
}

bool World::finalizeAddActor(ActorPrefab *prefab)
{
    debugAssert(prefab->getParentPrefab() == nullptr && delayInitPrefabs.contains(prefab));
    auto prefabItr = delayInitPrefabs.find(prefab);
    if (prefabItr != delayInitPrefabs.end())
    {
        delayInitPrefabs.erase(prefabItr);
        actorPrefabs.emplace_back(prefab);
        setupActorInternal(prefab, false);
        return true;
    }
    return false;
}

cbe::Actor *World::setupActorInternal(ActorPrefab *actorPrefab, bool bUpdateTfTree)
{
    Actor *actor = actorPrefab->getActorTemplate();
    if (EWorldState::isPlayState(worldState))
    {
        actors.emplace_back(actor);
    }
    ActorPrefab::initializeActor(actorPrefab);
    debugAssert(actor->getRootComponent() && !compToTf.contains(actor->getRootComponent()));

    // Inserting each TransformComponent into global TF tree
    for (TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        compToTf[actorTransformComp] = txHierarchy.add(ComponentWorldTF{ actorTransformComp, actorTransformComp->getRelativeTransform() });
    }
    // Updating its attachments, Reason for separated setup is that transformComps will not be ordered from root to leafs
    for (TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        // All none root must have its attachedTo setup at this point, Ignoring root component alone to not clear actorAttachedTo by mistake
        if (actorTransformComp != actor->getRootComponent())
        {
            debugAssertf(
                actorTransformComp->getAttachedTo(), "TransformComponent {} is not root and not attached!",
                actorTransformComp->getObjectData().name
            );
            updateTfAttachment(actorTransformComp, actorTransformComp->getAttachedTo(), false);
        }
    }
    // Now update the world transforms
    if (bUpdateTfTree)
    {
        TFHierarchyIdx rootCompIdx = compToTf[actor->getRootComponent()];
        std::vector<TFHierarchyIdx> idxsToUpdate;
        idxsToUpdate.emplace_back(rootCompIdx);
        txHierarchy.getChildren(idxsToUpdate, rootCompIdx, true);
        updateWorldTf(idxsToUpdate);
        // No need to broadcast transformed events as new add events will be triggered and transformed is just subset of add/remove
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
    for (TransformLeafComponent *actorLeafComp : actor->getLeafComponents())
    {
        broadcastLeafCompAdded(actorLeafComp);
    }
    return actorPrefab->getActorTemplate();
}

void World::updateTfAttachment(TransformComponent *attachingComp, TransformComponent *attachedTo, bool bUpdateTfTree)
{
    auto attachingItr = compToTf.find(attachingComp);
    debugAssert(attachingItr != compToTf.end());
    TFHierarchyIdx attachingIdx = attachingItr->second;

    if (attachedTo)
    {
        auto attachedToItr = compToTf.find(attachedTo);
        debugAssert(attachedToItr != compToTf.end());
        TFHierarchyIdx attachedToIdx = attachedToItr->second;

        txHierarchy.relinkTo(attachingIdx, attachedToIdx);
    }
    else
    {
        txHierarchy.relinkTo(attachingIdx);
    }

    if (bUpdateTfTree)
    {
        std::vector<TFHierarchyIdx> idxsToUpdate;
        idxsToUpdate.emplace_back(attachingIdx);
        txHierarchy.getChildren(idxsToUpdate, attachingIdx, true);
        updateWorldTf(idxsToUpdate);
    }
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
            WACHelpers::detachActor(actorToDetach);
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
    for (TransformLeafComponent *actorLeafComp : actor->getLeafComponents())
    {
        leafComponentRemoved(actor, actorLeafComp);
    }
    broadcastActorRemoved(actor);
}

} // namespace cbe