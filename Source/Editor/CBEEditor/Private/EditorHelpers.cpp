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
#include "ObjectTemplate.h"
#include "Classes/World.h"
#include "Classes/Actor.h"
#include "Classes/ActorPrefab.h"
#include "Classes/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

cbe::StaticMesh *
EditorHelpers::createStaticMesh(const String &packageName, const String &packagePath, const String &meshName, cbe::SMCreateInfo &&createInfo)
{
    cbe::Package *package = cbe::Package::createPackage(packageName, packagePath, false);
    if (package == nullptr)
    {
        return nullptr;
    }
    cbe::markDirty(package);
    SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);

    cbe::StaticMesh *mesh = cbe::create<cbe::StaticMesh, cbe::SMCreateInfo &&>(
        meshName, package, cbe::EObjectFlagBits::ObjFlag_PackageLoaded, std::forward<decltype(createInfo)>(createInfo)
    );
    debugAssert(mesh);

    return mesh;
}

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
        String smName{ sm->getObjectData().name };
        ActorPrefab *smActorPrefab = ActorPrefab::prefabFromActorTemplate(
            ActorPrefab::objectTemplateFromObj(addActorToWorld(world, cbe::Actor::staticType(), smName, 0))
        );
        Actor *smActor = smActorPrefab->getActorTemplate();
        StaticMeshComponent *smComp
            = static_cast<StaticMeshComponent *>(addComponentToPrefab(smActorPrefab, StaticMeshComponent::staticType(), smName));

        Object *modifyingComp
            = modifyPrefabCompField(PropertyHelper::findField(smComp->getType(), GET_MEMBER_ID_CHECKED(StaticMeshComponent, mesh)), smComp);
        debugAssert(modifyingComp == smComp);
        smComp->mesh = sm;
        // Attach static mesh to root even though in Prefab, added component will get attached to root by default
        smActorPrefab->setLeafAttachedTo(smComp, smActorPrefab->getRootComponent());

        cbe::WACHelpers::attachActor(smActor, rootActorRoot);

        debugAssert(world->actorAttachedTo[smActor].component == rootActorRoot);
    }
    cbe::markDirty(world);
    return rootActorPrefab->getActorTemplate();
}

cbe::Actor *EditorHelpers::addActorToWorld(cbe::World *world, CBEClass actorClass, const String &actorName, EObjectFlags flags)
{
    cbe::ActorPrefab *prefab
        = cbe::create<cbe::ActorPrefab, StringID, String>(actorName + TCHAR("_Prefab"), world, flags, actorClass->name, actorName);
    world->actorPrefabs.emplace_back(prefab);
    postAddActorToWorld(world, prefab);
    cbe::markDirty(world);
    return prefab->getActorTemplate();
}

cbe::Actor *EditorHelpers::addActorToWorld(cbe::World *world, cbe::ActorPrefab *inPrefab, const String &name, EObjectFlags flags)
{
    cbe::ActorPrefab *prefab = create<cbe::ActorPrefab, cbe::ActorPrefab *, String>(name + TCHAR("_Prefab"), world, flags, inPrefab, name);
    world->actorPrefabs.emplace_back(prefab);
    postAddActorToWorld(world, prefab);
    cbe::markDirty(world);
    return prefab->getActorTemplate();
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
    return modifiedComp;
}

cbe::Object *EditorHelpers::modifyPrefabCompField(const FieldProperty *prop, cbe::Object *comp)
{
    debugAssert(comp && prop);
    cbe::ObjectTemplate *compTemplate = nullptr;
    if (cbe::ActorPrefab::isNativeComponent(comp))
    {
        // Probably actor owned/native component
        compTemplate = cbe::ActorPrefab::objectTemplateFromNativeComp(comp);
        cbe::ActorPrefab *prefab = cbe::ActorPrefab::prefabFromActorTemplate(compTemplate);
        cbe::Object *newComp = modifyComponentInPrefab(prefab, comp);
        debugAssert(newComp == comp);
    }
    else
    {
        cbe::ActorPrefab *prefab = cbe::ActorPrefab::prefabFromCompTemplate(cbe::ActorPrefab::objectTemplateFromObj(comp));
        comp = modifyComponentInPrefab(prefab, comp);
        compTemplate = cbe::ActorPrefab::objectTemplateFromObj(comp);
    }
    debugAssert(compTemplate);
    compTemplate->onFieldModified(prop, comp);
    return comp;
}

cbe::Object *EditorHelpers::resetPrefabCompField(const FieldProperty *prop, cbe::Object *comp)
{
    debugAssert(comp && prop);
    cbe::ObjectTemplate *compTemplate = nullptr;
    if (cbe::ActorPrefab::isNativeComponent(comp))
    {
        // Probably actor owned/native component
        compTemplate = cbe::ActorPrefab::objectTemplateFromNativeComp(comp);
        cbe::ActorPrefab *prefab = cbe::ActorPrefab::prefabFromActorTemplate(compTemplate);
        cbe::Object *newComp = modifyComponentInPrefab(prefab, comp);
        debugAssert(newComp == comp);
    }
    else
    {
        cbe::ActorPrefab *prefab = cbe::ActorPrefab::prefabFromCompTemplate(cbe::ActorPrefab::objectTemplateFromObj(comp));
        comp = modifyComponentInPrefab(prefab, comp);
        compTemplate = cbe::ActorPrefab::objectTemplateFromObj(comp);
    }
    debugAssert(compTemplate);
    compTemplate->onFieldReset(prop, comp);
    return comp;
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
    for (cbe::TransformLeafComponent *actorLeafComp : actor->getLeafComponents())
    {
        componentAddedToWorld(world, actor, actorLeafComp);
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

    world->broadcastActorAdded(actor);
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
    cbe::WACHelpers::detachActor(actor);
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
    for (cbe::TransformLeafComponent *actorLeafComp : actor->getLeafComponents())
    {
        componentRemovedFromWorld(world, actor, actorLeafComp);
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
    world->broadcastActorRemoved(actor);
}

void EditorHelpers::componentAddedToWorld(cbe::World *world, cbe::Actor * /*actor*/, cbe::Object *component)
{
    if (PropertyHelper::isChildOf<cbe::TransformComponent>(component->getType()))
    {
        world->broadcastTfCompAdded(component);
    }
    else if (PropertyHelper::isChildOf<cbe::LogicComponent>(component->getType()))
    {
        world->broadcastLogicCompAdded(component);
    }
    else if (PropertyHelper::isChildOf<cbe::TransformLeafComponent>(component->getType()))
    {
        world->broadcastLeafCompAdded(component);
    }
    else
    {
        fatalAssertf(
            !"Invalid component", "Invalid component type {} added to the world {}", component->getType()->nameString,
            world->getObjectData().name
        );
    }
}

void EditorHelpers::componentRemovedFromWorld(cbe::World *world, cbe::Actor * /*actor*/, cbe::Object *component)
{
    // Just to be safe and not have any hanging references
    if (cbe::TransformComponent *tfComponent = cbe::cast<cbe::TransformComponent>(component))
    {
        auto compWorldTfItr = world->compToTf.find(tfComponent);
        cbe::World::TFHierarchyIdx compTfIdx;
        if (compWorldTfItr != world->compToTf.end())
        {
            compTfIdx = compWorldTfItr->second;
#if DEBUG_VALIDATIONS
            std::vector<cbe::World::TFHierarchyIdx> directAttachments;
            world->txHierarchy.getChildren(directAttachments, compTfIdx, false);
            // So if ever below assert fails. There is desync in logic on how world components are attached and tree updates
            debugAssert(directAttachments.empty());
#endif // DEBUG_VALIDATIONS

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
        world->broadcastTfCompRemoved(component);
    }
    else if (PropertyHelper::isChildOf<cbe::LogicComponent>(component->getType()))
    {
        world->broadcastLogicCompRemoved(component);
    }
    else if (PropertyHelper::isChildOf<cbe::TransformLeafComponent>(component->getType()))
    {
        world->broadcastLeafCompRemoved(component);
    }
    else
    {
        fatalAssertf(
            !"Invalid component", "Invalid component type {} removed from the world {}", component->getType()->nameString,
            world->getObjectData().name
        );
    }
}
