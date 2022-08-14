/*!
 * \file EditorHelpers.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EditorHelpers.h"
#include "CBEPackage.h"
#include "CBEObjectHelpers.h"
#include "Classes/World.h"
#include "Classes/Actor.h"
#include "Classes/ActorPrefab.h"
#include "Classes/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

cbe::Actor *
    EditorHelpers::addStaticMeshesToWorld(const std::vector<cbe::StaticMesh *> &staticMeshes, cbe::World *world, const String &rootActorName)
{
    using namespace cbe;
    if (rootActorName.empty())
    {
        LOG_WARN("EditorHelpers", "Root actor name must be valid! Cannot add static meshes to the world");
        return nullptr;
    }
    if (staticMeshes.empty() || world == nullptr)
    {
        LOG_WARN("EditorHelpers", "World or staticMeshes are invalid! Cannot add static meshes to the world");
        return nullptr;
    }

    // Create world root actor for this list of static meshes
    ActorPrefab *rootActorPrefab = ActorPrefab::prefabFromActorTemplate(
        ActorPrefab::objectTemplateFromObj(addActorToWorld(world, cbe::Actor::staticType(), rootActorName, 0))
    );
    TransformComponent *rootActorRoot = rootActorPrefab->getRootComponent();
    debugAssert(rootActorRoot);

    for (StaticMesh *sm : staticMeshes)
    {
        ActorPrefab *smActorPrefab = ActorPrefab::prefabFromActorTemplate(
            ActorPrefab::objectTemplateFromObj(addActorToWorld(world, cbe::Actor::staticType(), sm->getName(), 0))
        );
        Actor *smActor = smActorPrefab->getActorTemplate();
        StaticMeshComponent *smComp
            = static_cast<StaticMeshComponent *>(addComponentToPrefab(smActorPrefab, StaticMeshComponent::staticType(), sm->getName()));

        Object *modifyingComp = modifyPrefabCompField(PropertyHelper::findField(smComp->getType(), GET_MEMBER_ID_CHECKED(StaticMeshComponent, mesh)), smComp);
        debugAssert(modifyingComp == smComp);
        smComp->mesh = sm;

        // Reset root and remove default root component
        TransformComponent *defaultRoot = smActorPrefab->getRootComponent();
        smActorPrefab->setRootComponent(smComp);
        smActorPrefab->removeComponent(defaultRoot);

        attachActorInWorld(world, smActor, rootActorRoot);

        debugAssert(smActor->getActorAttachedTo() == nullptr && world->actorAttachedTo[smActor].component == rootActorRoot);
    }
    cbe::markDirty(world);
    return rootActorPrefab->getActorTemplate();
}

cbe::Object *EditorHelpers::addComponentToPrefab(cbe::ActorPrefab *prefab, CBEClass compClass, const String &compName)
{
    cbe::Object *comp = prefab->addComponent(compClass, compName);
    if (cbe::World *world = prefab->getActorTemplate()->getWorld())
    {
        componentAddedToWorld(world, prefab->getActorTemplate(), comp);
        cbe::markDirty(world);
    }
    return comp;
}

cbe::Object *EditorHelpers::addComponentToPrefab(cbe::ActorPrefab *prefab, cbe::ObjectTemplate *compTemplate, const String &compName)
{
    cbe::Object *comp = prefab->addComponent(compTemplate, compName);
    if (cbe::World *world = prefab->getActorTemplate()->getWorld())
    {
        componentAddedToWorld(world, prefab->getActorTemplate(), comp);
        cbe::markDirty(world);
    }
    return comp;
}

void EditorHelpers::removeComponentFromPrefab(cbe::ActorPrefab *prefab, cbe::Object *comp)
{
    if (cbe::World *world = prefab->getActorTemplate()->getWorld())
    {
        componentRemovedFromWorld(world, prefab->getActorTemplate(), comp);
        cbe::markDirty(world);
    }
    prefab->removeComponent(comp);
}

cbe::Object *EditorHelpers::modifyComponentInPrefab(cbe::ActorPrefab *prefab, cbe::Object *modifyingComp)
{
    cbe::Object *modifiedComp = prefab->modifyComponent(modifyingComp);
    // Should I have this special function? Right now the purpose is only to update data in world on modifying
    // However prefabs in world will always have overrides at start and always will modifiedComp == modifyingComp
    cbe::World *world = prefab->getActorTemplate()->getWorld();
    if (world && modifiedComp != modifyingComp)
    {
        if (cbe::TransformComponent *tfComponent = cbe::cast<cbe::TransformComponent>(modifiedComp))
        {
            for (auto actorAttachedToItr = world->actorAttachedTo.begin(); actorAttachedToItr != world->actorAttachedTo.end();
                 ++actorAttachedToItr)
            {
                if (actorAttachedToItr->second.component == modifyingComp)
                {
                    actorAttachedToItr->second.component = tfComponent;
                }
            }
            cbe::markDirty(world);
        }
    }
    return modifiedComp;
}

cbe::Object *EditorHelpers::modifyPrefabCompField(const FieldProperty *prop, cbe::Object *comp)
{
    debugAssert(comp && prop);
    cbe::ObjectTemplate *compTemplate = cbe::ActorPrefab::objectTemplateFromObj(comp);
    cbe::ActorPrefab *prefab = cbe::ActorPrefab::prefabFromCompTemplate(compTemplate);
    debugAssert(prefab);

    comp = modifyComponentInPrefab(prefab, comp);
    compTemplate = cbe::ActorPrefab::objectTemplateFromObj(comp);
    compTemplate->onFieldModified(prop, comp);
    return comp;
}

cbe::Object *EditorHelpers::resetPrefabCompField(const FieldProperty *prop, cbe::Object *comp)
{
    debugAssert(comp && prop);
    cbe::ObjectTemplate *compTemplate = cbe::ActorPrefab::objectTemplateFromObj(comp);
    cbe::ActorPrefab *prefab = cbe::ActorPrefab::prefabFromCompTemplate(compTemplate);
    debugAssert(prefab);

    comp = modifyComponentInPrefab(prefab, comp);
    compTemplate = cbe::ActorPrefab::objectTemplateFromObj(comp);
    compTemplate->onFieldReset(prop, comp);
    return comp;
}

cbe::Actor *EditorHelpers::addActorToWorld(cbe::World *world, CBEClass actorClass, const String &actorName, EObjectFlags flags)
{
    cbe::ActorPrefab *prefab
        = cbe::create<cbe::ActorPrefab, StringID, const String &>(actorName + TCHAR("_Prefab"), world, flags, actorClass->name, actorName);
    world->actorPrefabs.emplace_back(prefab);
    postAddActorToWorld(world, prefab);
    cbe::markDirty(world);
    return prefab->getActorTemplate();
}

cbe::Actor *EditorHelpers::addActorToWorld(cbe::World *world, cbe::ActorPrefab *inPrefab, const String &name, EObjectFlags flags)
{
    cbe::ActorPrefab *prefab
        = create<cbe::ActorPrefab, cbe::ActorPrefab *, const String &>(name + TCHAR("_Prefab"), world, flags, inPrefab, name);
    world->actorPrefabs.emplace_back(prefab);
    postAddActorToWorld(world, prefab);
    cbe::markDirty(world);
    return prefab->getActorTemplate();
}

void EditorHelpers::postAddActorToWorld(cbe::World *world, cbe::ActorPrefab *prefab)
{
    cbe::Actor *actor = prefab->getActorTemplate();
    for (cbe::TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        componentAddedToWorld(world, actor, actorTransformComp);
    }
    for (cbe::LogicComponent *actorLogicComp : actor->getLogicComponents())
    {
        componentAddedToWorld(world, actor, actorLogicComp);
    }
    for (cbe::ObjectTemplate *compTemplate : prefab->getPrefabComponents())
    {
        componentAddedToWorld(world, actor, compTemplate->getTemplate());
    }
    for (const cbe::ActorPrefab::ComponentOverrideInfo &overrideInfo : prefab->getOverridenComponents())
    {
        debugAssert(overrideInfo.overriddenTemplate && overrideInfo.overriddenTemplate->getTemplate());
        componentAddedToWorld(world, actor, overrideInfo.overriddenTemplate->getTemplate());
    }
}

void EditorHelpers::removeActorFromWorld(cbe::World *world, cbe::Actor *actor)
{
    cbe::ObjectTemplate *actorTemplate = cbe::ActorPrefab::objectTemplateFromObj(actor);
    cbe::ActorPrefab *prefab = cbe::ActorPrefab::prefabFromActorTemplate(actorTemplate);
    if (prefab)
    {
        std::erase(world->actorPrefabs, prefab);
    }
    std::erase(world->actors, actor);
    detachActorInWorld(world, actor);
    // Detach anything that is attached to this actor
    for (auto actorAttachedToItr = world->actorAttachedTo.begin(); actorAttachedToItr != world->actorAttachedTo.end();)
    {
        if (actorAttachedToItr->second.actor == actor)
        {
            cbe::Actor *actorToDetach = actorAttachedToItr->first;
            actorAttachedToItr = world->actorAttachedTo.erase(actorAttachedToItr);
            // Actor in editor world will not be attached already in component, It happens at play start
            // So if ever below assert fails. There is desync in logic on how world actors are attached
            cbe::ActorPrefab *prefabToDetach
                = cbe::ActorPrefab::prefabFromActorTemplate(cbe::ActorPrefab::objectTemplateFromObj(actorToDetach));
            debugAssert(prefabToDetach->getRootComponent()->getAttachedTo() == nullptr);
        }
        else
        {
            ++actorAttachedToItr;
        }
    }
    for (cbe::TransformComponent *actorTransformComp : actor->getTransformComponents())
    {
        componentRemovedFromWorld(world, actor, actorTransformComp);
    }
    for (cbe::LogicComponent *actorLogicComp : actor->getLogicComponents())
    {
        componentRemovedFromWorld(world, actor, actorLogicComp);
    }
    for (cbe::ObjectTemplate *compTemplate : prefab->getPrefabComponents())
    {
        componentRemovedFromWorld(world, actor, compTemplate->getTemplate());
    }
    for (const cbe::ActorPrefab::ComponentOverrideInfo &overrideInfo : prefab->getOverridenComponents())
    {
        debugAssert(overrideInfo.overriddenTemplate && overrideInfo.overriddenTemplate->getTemplate());
        componentRemovedFromWorld(world, actor, overrideInfo.overriddenTemplate->getTemplate());
    }
}

void EditorHelpers::componentAddedToWorld(cbe::World *world, cbe::Actor *actor, cbe::Object *component)
{
    // Nothing to do now
}

void EditorHelpers::componentRemovedFromWorld(cbe::World *world, cbe::Actor *actor, cbe::Object *component)
{
    // Just to be safe and not have any hanging references
    cbe::TransformComponent *tfComponent = cbe::cast<cbe::TransformComponent>(component);
    if (tfComponent)
    {
        auto compWorldTfItr = world->compToTf.find(tfComponent);
        cbe::World::TFHierarchyIdx compTfIdx;
        if (compWorldTfItr != world->compToTf.end())
        {
            compTfIdx = compWorldTfItr->second;
            std::vector<cbe::World::TFHierarchyIdx> directAttachments;
            world->txHierarchy.getChildren(directAttachments, compTfIdx, false);

            // So if ever below assert fails. There is desync in logic on how world components are attached and tree updates
            debugAssert(directAttachments.empty());
            world->txHierarchy.remove(compTfIdx);
            world->compToTf.erase(compWorldTfItr);
        }

        // Detach anything that is attached to this component
        for (auto actorAttachedToItr = world->actorAttachedTo.begin(); actorAttachedToItr != world->actorAttachedTo.end();)
        {
            if (actorAttachedToItr->second.component == component)
            {
                cbe::Actor *actorToDetach = actorAttachedToItr->first;
                actorAttachedToItr = world->actorAttachedTo.erase(actorAttachedToItr);
                // Actor in editor world will not be attached already in component, It happens at play start
                // So if ever below assert fails. There is desync in logic on how world actors are attached
                cbe::ActorPrefab *prefabToDetach
                    = cbe::ActorPrefab::prefabFromActorTemplate(cbe::ActorPrefab::objectTemplateFromObj(actorToDetach));
                debugAssert(prefabToDetach->getRootComponent()->getAttachedTo() == nullptr);
            }
            else
            {
                ++actorAttachedToItr;
            }
        }
    }
}

void EditorHelpers::attachActorInWorld(cbe::World *world, cbe::Actor *attachingActor, cbe::TransformComponent *attachToComp)
{
    debugAssert(world && attachingActor && attachToComp);
    world->actorAttachedTo[attachingActor] = cbe::World::ActorAttachedToInfo{ attachToComp->getActor(), attachToComp };
}

void EditorHelpers::detachActorInWorld(cbe::World *world, cbe::Actor *detachingActor)
{
    debugAssert(world && detachingActor);
    world->actorAttachedTo.erase(detachingActor);
}
