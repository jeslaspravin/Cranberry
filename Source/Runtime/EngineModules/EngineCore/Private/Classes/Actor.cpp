/*!
 * \file Actor.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Classes/Actor.h"
#include "Classes/World.h"
#include "Serialization/ObjectSerializationHelpers.h"
#include "Classes/ActorPrefab.h"
#include "CBEObjectHelpers.h"
#include "ObjectTemplate.h"
#include "Components/ComponentBase.h"

namespace cbe
{

//////////////////////////////////////////////////////////////////////////
// ActorPrefab impl
//////////////////////////////////////////////////////////////////////////

inline constexpr static const uint32 ACTOR_PREFAB_SERIALIZER_VERSION = 0;
inline constexpr static const uint32 ACTOR_PREFAB_SERIALIZER_CUTOFF_VERSION = 0;
inline STRINGID_CONSTEXPR static const StringID ACTOR_PREFAB_CUSTOM_VERSION_ID = STRID("ActorPrefabSerializer");

ActorPrefab::ActorPrefab(StringID className, const String &actorName)
    : parentPrefab(nullptr)
{
    actorClass = IReflectionRuntimeModule::get()->getClassType(className);
    actorTemplate = create<ObjectTemplate, StringID, const String &>(actorName, this, getFlags(), className, actorName);

    // If there is no root component already then we must create a root component and add it
    if (!static_cast<Actor *>(actorTemplate->getTemplate())->getRootComponent())
    {
        String componentName = TCHAR("RootComponent");
        components.emplace_back(create<ObjectTemplate, StringID, const String &>(
            componentName, actorTemplate->getTemplate(), getFlags(), TransformComponent::staticType()->name, componentName
        ));
        rootComponent = static_cast<TransformComponent *>(components.back()->getTemplate());
    }
    markDirty(this);
}

ActorPrefab::ActorPrefab(ActorPrefab *inPrefab, const String &name)
    : parentPrefab(inPrefab)
{
    debugAssert(parentPrefab);
    actorClass = parentPrefab->actorClass;

    actorTemplate = create<ObjectTemplate, ObjectTemplate *, const String &>(name, this, getFlags(), parentPrefab->actorTemplate, name);
    // Since parentPrefab must have set it up
    debugAssert(static_cast<Actor *>(actorTemplate->getTemplate())->getRootComponent());

    // Fill component overrides structs
    componentOverrides.reserve(parentPrefab->components.size() + parentPrefab->componentOverrides.size());
    for (ObjectTemplate *parentPrefabComp : parentPrefab->components)
    {
        componentOverrides.emplace_back(parentPrefabComp, nullptr, nullptr);
    }
    for (const ComponentOverrideInfo &parentPrefabOverrides : parentPrefab->componentOverrides)
    {
        componentOverrides.emplace_back(
            parentPrefabOverrides.baseTemplate,
            parentPrefabOverrides.overriddenTemplate ? parentPrefabOverrides.overriddenTemplate : parentPrefabOverrides.lastOverride, nullptr
        );
    }
    // Fill component attached to components
    for (const std::pair<TransformComponent *const, TransformComponent *> &compAttachedTo : parentPrefab->componentAttachedTo)
    {
        if (isNativeComponent(compAttachedTo.second))
        {
            // Path of native component from this prefab's actor template
            String attachedToPath = ObjectPathHelper::getFullPath(compAttachedTo.second->getName().getChar(), actorTemplate->getTemplate());
            Object *nativeComp = get(attachedToPath.getChar());
            debugAssert(nativeComp && PropertyHelper::isChildOf<TransformComponent>(nativeComp->getType()));
            componentAttachedTo[compAttachedTo.first] = static_cast<TransformComponent *>(nativeComp);
        }
        else
        {
            componentAttachedTo.emplace(compAttachedTo);
        }
    }

    // TODO(Jeslas) : Should I really do this in world? I can avoid doing this if I recreate entire actor and its component at runtime.
    // This is mainly to use actor prefab directly in world/level without constructing a new actor from prefab. Makes it easier in editor
    bool bIsInWorld = cast<World>(getOuter());
    if (bIsInWorld)
    {
        // Do not have to replace actor inner components as they will already be replaced in deep copy
        std::unordered_map<Object *, Object *> objectReplacements;
        for (ComponentOverrideInfo &overrideInfo : parentPrefab->componentOverrides)
        {
            ObjectTemplate *templateToOverride = getTemplateToOverride(overrideInfo);
            createComponentOverride(overrideInfo, false);
            debugAssert(overrideInfo.overriddenTemplate);
            objectReplacements[templateToOverride->getTemplate()] = overrideInfo.overriddenTemplate;
        }
        replaceObjectReferences(this, objectReplacements, EObjectTraversalMode::EntireObjectTree);
    }
    markDirty(this);
}

cbe::Object *ActorPrefab::modifyComponent(Object *modifyingComp)
{
    if (!isOwnedComponent(modifyingComp))
    {
        ObjectTemplate *modifyingCompTemplate = objectTemplateFromObj(modifyingComp);
        debugAssert(modifyingCompTemplate);
        auto compOverrideItr = std::find_if(
            componentOverrides.begin(), componentOverrides.end(),
            [modifyingCompTemplate](const ComponentOverrideInfo &overrideInfo)
            {
                return getTemplateToOverride(overrideInfo) == modifyingCompTemplate;
            }
        );
        // Why calling modify on previous component if overridden template exists?
        debugAssert(compOverrideItr != componentOverrides.end() && compOverrideItr->overriddenTemplate == nullptr);

        ComponentOverrideInfo &modifyingOverrideInfo = *compOverrideItr;
        ObjectTemplate *modifyingTemplate = getTemplateToOverride(modifyingOverrideInfo);
        ActorPrefab *modifyingPrefab = prefabFromCompTemplate(modifyingTemplate);

        std::unordered_map<Object *, Object *> replacements;
        createComponentOverride(modifyingOverrideInfo, false);
        replacements[modifyingComp] = modifyingOverrideInfo.overriddenTemplate->getTemplate();

        // Find all the components that refers component to override from its ActorPrefab.
        // Recursively do it until no more overrides are referred
        std::vector<ObjectReferences> references
            = findObjectReferences(modifyingPrefab, { modifyingComp }, EObjectTraversalMode::EntireObjectTree);
        while (!references.empty())
        {
            std::vector<ObjectReferences> newReferences;
            for (const ObjectReferences &objRef : references)
            {
                // Native components already has overrides inside actorTemplate, Anything other than component is ignored
                if (isNativeComponent(objRef.foundInObject)
                    || !(
                        PropertyHelper::isChildOf<LogicComponent>(objRef.foundInObject->getType())
                        || PropertyHelper::isChildOf<TransformComponent>(objRef.foundInObject->getType())
                    ))
                {
                    continue;
                }
                ObjectTemplate *foundInTemplate = objectTemplateFromObj(objRef.foundInObject);
                ActorPrefab *foundInPrefab = prefabFromCompTemplate(foundInTemplate);
                debugAssert(modifyingPrefab == foundInPrefab);

                // Find the BaseComponent template to find in this ActorPrefab, its corresponding override info
                auto foundInOverrideInfoItr = std::find_if(
                    foundInPrefab->componentOverrides.begin(), foundInPrefab->componentOverrides.end(),
                    [foundInTemplate](const ComponentOverrideInfo &overrideInfo)
                    {
                        return overrideInfo.overriddenTemplate == foundInTemplate;
                    }
                );
                // If not valid then foundInTemplate is base template
                ObjectTemplate *foundInBaseTemplate = foundInTemplate;
                if (foundInOverrideInfoItr != foundInPrefab->componentOverrides.end())
                {
                    foundInBaseTemplate = foundInOverrideInfoItr->baseTemplate;
                }

                // BaseTemplate is found, Find override info to create in this ActorPrefab
                auto overrideItr = std::find_if(
                    componentOverrides.begin(), componentOverrides.end(),
                    [foundInBaseTemplate](const ComponentOverrideInfo &overrideInfo)
                    {
                        return overrideInfo.baseTemplate == foundInBaseTemplate;
                    }
                );
                debugAssert(overrideItr != componentOverrides.end());
                // If we have not created override for this referrer already
                if (overrideItr->overriddenTemplate == nullptr)
                {
                    ObjectTemplate *overridingTemplate = getTemplateToOverride(*overrideItr);
                    createComponentOverride(*overrideItr, false);
                    replacements[overridingTemplate->getTemplate()] = overrideItr->overriddenTemplate->getTemplate();

                    std::vector<ObjectReferences> additionalReferences = findObjectReferences(
                        prefabFromCompTemplate(overridingTemplate), { overridingTemplate->getTemplate() },
                        EObjectTraversalMode::EntireObjectTree
                    );
                    newReferences.insert(newReferences.end(), additionalReferences.begin(), additionalReferences.end());
                }
            }
            references = std::move(newReferences);
        }
        replaceObjectReferences(this, replacements, EObjectTraversalMode::EntireObjectTree);
        markDirty(this);
        return modifyingOverrideInfo.overriddenTemplate->getTemplate();
    }
    markDirty(this);
    return modifyingComp;
}

void ActorPrefab::setRootComponent(TransformComponent *component)
{
    if (!isOwnedComponent(component))
    {
        LOG_WARN("ActorPrefab", "Component is not owned, Call modifyComponent() to override the component!", component->getFullPath());
        return;
    }
    debugAssert(canOverrideRootComp());

    TransformComponent *rootComp = getRootComponent();
    // Do not replace native component
    if (rootComp == component || isNativeComponent(rootComp))
    {
        return;
    }
    componentAttachedTo.erase(component);
    component->setAttachedTo(nullptr);
    rootComponent = component;
    // if current root component is the component that setting root component overrides we should not attach current to new
    auto compOverrideInfoItr = std::find_if(
        componentOverrides.begin(), componentOverrides.end(),
        [rootComp](const ComponentOverrideInfo &overrideInfo)
        {
            return getTemplateToOverride(overrideInfo)->getTemplate() == rootComp;
        }
    );
    if (rootComp && (compOverrideInfoItr == componentOverrides.end() || compOverrideInfoItr->overriddenTemplate->getTemplate() != component))
    {
        componentAttachedTo[rootComp] = component;
        if (isOwnedComponent(rootComp))
        {
            rootComp->setAttachedTo(component);
        }
    }
    markDirty(this);
}

void ActorPrefab::setComponentAttachedTo(TransformComponent *attachingComp, TransformComponent *attachedToComp)
{
    if (!isOwnedComponent(attachingComp))
    {
        LOG_WARN(
            "ActorPrefab", "Attaching component is not owned, Call modifyComponent() to override the component!", attachingComp->getFullPath()
        );
        return;
    }
    debugAssert(attachingComp);

    TransformComponent *rootComp = getRootComponent();
    if (rootComp == attachingComp)
    {
        alertAlwaysf(
            rootComp != attachingComp,
            "Cannot attach root component to something else. Use setRootComponent() if want to replace root component"
        );
        return;
    }

    if (attachedToComp == nullptr)
    {
        componentAttachedTo.erase(attachingComp);
        attachingComp->setAttachedTo(nullptr);
    }
    else
    {
        componentAttachedTo[attachingComp] = attachedToComp;
        attachingComp->setAttachedTo(attachedToComp);
    }
}

Object *ActorPrefab::addComponent(CBEClass compClass, const String &compName)
{
    ObjectTemplate *compTemplate
        = create<ObjectTemplate, StringID, const String &>(compName, actorTemplate->getTemplate(), getFlags(), compClass->name, compName);
    components.emplace_back(compTemplate);
    postAddComponent(compTemplate->getTemplate());
    return compTemplate->getTemplate();
}

Object *ActorPrefab::addComponent(ObjectTemplate *compTemplate, const String &compName)
{
    ObjectTemplate *compObjTemplate
        = create<ObjectTemplate, ObjectTemplate *, const String &>(compName, actorTemplate->getTemplate(), getFlags(), compTemplate, compName);
    components.emplace_back(compObjTemplate);
    postAddComponent(compObjTemplate->getTemplate());
    return compObjTemplate->getTemplate();
}

void ActorPrefab::removeComponent(Object *comp)
{
    debugAssert(!isNativeComponent(comp));
    TransformComponent *tfComponent = cast<TransformComponent>(comp);
    ObjectTemplate *compTemplate = objectTemplateFromObj(comp);
    debugAssert(compTemplate);
    auto compTemplateItr = std::find(components.begin(), components.end(), compTemplate);
    if (compTemplateItr == components.end())
    {
        LOG("ActorPrefab", "Component %s is already removed", comp->getName());
        return;
    }

    if (tfComponent)
    {
        bool bReplaceRoot = (rootComponent == tfComponent);
        // component to attach the components attached to tfComponent
        TransformComponent *reattachTo = nullptr;
        if (bReplaceRoot)
        {
            // If this is the root tfComponent will not be attached to anything, so find previous root component
            ActorPrefab *actorPrefab = parentPrefab;
            while ((reattachTo == nullptr) && actorPrefab)
            {
                reattachTo = actorPrefab->rootComponent;
            }
            // If this is the first root component then replace root with one of attached component
            if (reattachTo == nullptr)
            {
                for (std::pair<TransformComponent *const, TransformComponent *> &attachedToPair : componentAttachedTo)
                {
                    if (attachedToPair.second == tfComponent)
                    {
                        reattachTo = attachedToPair.first;
                        setRootComponent(reattachTo);
                        // Remove tfComponent since setting root comp will reattach tfComponent to new root
                        setComponentAttachedTo(tfComponent, nullptr);
                        break;
                    }
                }
            }
        }
        else
        {
            auto tfAttachedToItr = componentAttachedTo.find(tfComponent);
            debugAssert(tfAttachedToItr != componentAttachedTo.end());
            reattachTo = tfAttachedToItr->second;
            componentAttachedTo.erase(tfAttachedToItr);
        }

        // reattachTo will be null only if there is no more TransformComponent
        if (reattachTo)
        {
            for (std::pair<TransformComponent *const, TransformComponent *> &attachedToPair : componentAttachedTo)
            {
                if (attachedToPair.second == tfComponent)
                {
                    // Map will not change since only value will be changed
                    setComponentAttachedTo(attachedToPair.first, reattachTo);
                }
            }
        }
    }

    // Replace anything that used this component to null at least in this ActorPrefab. Derived prefab must handle it them self
    std::unordered_map<Object *, Object *> replacements = {
        {comp, nullptr}
    };
    replaceObjectReferences(this, replacements, EObjectTraversalMode::EntireObjectTree);
    markDirty(this);
    compTemplate->beginDestroy();
    comp->beginDestroy();
}

TransformComponent *ActorPrefab::getRootComponent() const
{
    const ActorPrefab *rootCompFromPrefab = this;
    while (rootCompFromPrefab && !rootCompFromPrefab->rootComponent)
    {
        rootCompFromPrefab = rootCompFromPrefab->parentPrefab;
    }
    if (rootCompFromPrefab)
    {
        return rootCompFromPrefab->rootComponent;
    }

    debugAssert(actorTemplate->getTemplateAs<Actor>()->rootComponent);
    return actorTemplate->getTemplateAs<Actor>()->rootComponent;
}

ObjectArchive &ActorPrefab::serialize(ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        uint32 dataVersion = ar.getCustomVersion(uint32(ACTOR_PREFAB_CUSTOM_VERSION_ID));
        // This must crash
        fatalAssertf(
            ACTOR_PREFAB_SERIALIZER_CUTOFF_VERSION >= dataVersion,
            "Version of ActorPrefab %u loaded from package %s is outdated, Minimum supported %u!", dataVersion, getOuterMost()->getFullPath(),
            ACTOR_PREFAB_SERIALIZER_CUTOFF_VERSION
        );
    }
    else
    {
        ar.setCustomVersion(uint32(ACTOR_PREFAB_CUSTOM_VERSION_ID), ACTOR_PREFAB_SERIALIZER_VERSION);
    }

    ar << actorClass;
    ObjectSerializationHelpers::serializeAllFields(this, ar);

    if (ar.isLoading())
    {
        if (parentPrefab == nullptr)
        {
            componentOverrides.clear();
        }
        alertAlwaysf(actorClass && actorTemplate, "Missing actor class/Template for actor prefab %s", getFullPath());
        std::erase(components, nullptr);
        std::erase_if(
            componentOverrides,
            [](const ComponentOverrideInfo &overrideInfo)
            {
                if (overrideInfo.baseTemplate == nullptr)
                {
                    if (overrideInfo.overriddenTemplate)
                    {
                        overrideInfo.overriddenTemplate->beginDestroy();
                    }
                    return true;
                }
                return false;
            }
        );
        // Fixing up attachments and detached components
        componentAttachedTo.erase(nullptr);
        // First make all invalid attached to components as null or connect to a valid component up in hierarchy
        for (std::pair<TransformComponent *const, TransformComponent *> &compAttachedToPair : componentAttachedTo)
        {
            if (compAttachedToPair.second != nullptr && !isValid(compAttachedToPair.second))
            {
                // walk the tree and find first valid attachable component
                auto nextAttachedToItr = componentAttachedTo.find(compAttachedToPair.second);
                while (nextAttachedToItr != componentAttachedTo.end())
                {
                    // If reached nullptr then we lost the chain leaving the loop is better
                    if (nextAttachedToItr->second == nullptr || isValid(nextAttachedToItr->second))
                    {
                        break;
                    }
                    nextAttachedToItr = componentAttachedTo.find(nextAttachedToItr->second);
                }
                if (nextAttachedToItr != componentAttachedTo.end())
                {
                    compAttachedToPair.second = nextAttachedToItr->second;
                    compAttachedToPair.first->setAttachedTo(nextAttachedToItr->second);
                }
                else
                {
                    compAttachedToPair.second = nullptr;
                    compAttachedToPair.first->setAttachedTo(nullptr);
                }
            }
        }
        // Setup root component if it is lost
        if (rootComponent == nullptr && getRootComponent() == nullptr)
        {
            // Go through each connection and find longest alive chain. Root of it will be the new fixed up root
            TransformComponent *possibleRoot = nullptr;
            int32 hierarchyDepth = -1;
            for (std::pair<TransformComponent *const, TransformComponent *> &compAttachedToPair : componentAttachedTo)
            {
                int32 depth = compAttachedToPair.second ? 1 : 0;
                TransformComponent *attachedToComp = compAttachedToPair.second;
                while (attachedToComp)
                {
                    auto nextAttachedToItr = componentAttachedTo.find(attachedToComp);
                    if (nextAttachedToItr != componentAttachedTo.end())
                    {
                        attachedToComp = nextAttachedToItr->second;
                        depth++;
                    }
                    else
                    {
                        attachedToComp = nullptr;
                    }
                }
                if (depth > hierarchyDepth)
                {
                    possibleRoot = compAttachedToPair.first;
                }
            }

            // If there is no possible root then it means there is no TransformComponent
            if (possibleRoot)
            {
                // We cannot set not owned component as root
                // Not owned but no root this is not possible unless something is really messed up
                debugAssert(isOwnedComponent(possibleRoot));
                rootComponent = possibleRoot;
                possibleRoot->setAttachedTo(nullptr);
                componentAttachedTo.erase(rootComponent);
            }
            else
            {
                componentAttachedTo.clear();
                rootComponent = nullptr;
            }
        }
        // Now connect all the stranded components to root
        TransformComponent *rootComp = getRootComponent();
        for (std::pair<TransformComponent *const, TransformComponent *> &compAttachedToPair : componentAttachedTo)
        {
            if (compAttachedToPair.second == nullptr)
            {
                compAttachedToPair.second = rootComp;
                compAttachedToPair.first->setAttachedTo(rootComp);
            }
        }
    }
    return ar;
}

void ActorPrefab::initializeActor(ActorPrefab *inPrefab)
{
    World *actorWorld = inPrefab->getActorTemplate()->getWorld();
    debugAssert(actorWorld);

    Actor *actor = inPrefab->getActorTemplate();
    uint32 nativeCompsCount = uint32(actor->getLogicComponents().size() + actor->getTransformComponents().size());
    actor->rootComponent = inPrefab->getRootComponent();
    auto addCompToActor = [&actor, &inPrefab](Object *comp)
    {
        if (TransformComponent *tfComp = cast<TransformComponent>(comp))
        {
#if DEV_BUILD
            if (tfComp != actor->rootComponent)
            {
                auto attachedToItr = inPrefab->componentAttachedTo.find(tfComp);
                alertAlways(attachedToItr != inPrefab->componentAttachedTo.end() && attachedToItr->second == tfComp->getAttachedTo());
            }
#endif
            actor->transformComps.insert(tfComp);
        }
        else if (LogicComponent *logicComp = cast<LogicComponent>(comp))
        {
            actor->logicComps.insert(logicComp);
        }
        else
        {
            fatalAssertf(false, "Why?? Component %s of type %s is not a valid component", comp->getName(), comp->getType()->nameString);
        }
    };
    for (Object *comp : inPrefab->components)
    {
        addCompToActor(comp);
    }
    for (const ComponentOverrideInfo &overrideInfo : inPrefab->componentOverrides)
    {
        addCompToActor(overrideInfo.overriddenTemplate->getTemplate());
    }
    debugAssert(
        actor->rootComponent
        && (actor->logicComps.size() + actor->transformComps.size())
               == (inPrefab->components.size() + inPrefab->componentOverrides.size() + nativeCompsCount)
    );
}

FORCE_INLINE bool ActorPrefab::isNativeComponent(Object *obj) const
{
    return obj && PropertyHelper::isChildOf<Actor>(obj->getOuter()->getType());
}

void ActorPrefab::createComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences)
{
    ObjectTemplate *componentTemplate = getTemplateToOverride(overrideInfo);
    TransformComponent *tfComponent = componentTemplate->getTemplateAs<TransformComponent>();

#if DEBUG_BUILD
    ActorPrefab *actorPrefab = prefabFromCompTemplate(componentTemplate);
    LogicComponent *logicComponent = componentTemplate->getTemplateAs<LogicComponent>();
    debugAssert(componentTemplate && (logicComponent || tfComponent) && actorPrefab && actorPrefab != this);
#endif

    overrideInfo.overriddenTemplate = create<ObjectTemplate, ObjectTemplate *, const String &>(
        componentTemplate->getName(), actorTemplate->getTemplate(), componentTemplate->getFlags(), componentTemplate,
        componentTemplate->getName()
    );

    if (tfComponent)
    {
        TransformComponent *tfCompOverride = static_cast<TransformComponent *>(overrideInfo.overriddenTemplate->getTemplate());
        if (getRootComponent() == tfComponent && rootComponent == nullptr)
        {
            rootComponent = tfCompOverride;
        }
        else
        {
            auto tfAttachedToItr = componentAttachedTo.find(tfComponent);
            debugAssert(tfAttachedToItr != componentAttachedTo.end());
            setComponentAttachedTo(tfCompOverride, tfAttachedToItr->second);
            setComponentAttachedTo(tfComponent, nullptr);
        }

        // Replace all the component that attached to previous component to be attached to new override
        for (const std::pair<TransformComponent *const, TransformComponent *> &attachedToPair : componentAttachedTo)
        {
            if (attachedToPair.second == tfComponent)
            {
                // Map will not change since only value will be changed
                setComponentAttachedTo(attachedToPair.first, tfCompOverride);
            }
        }
    }

    if (bReplaceReferences)
    {
        std::unordered_map<Object *, Object *> replacements = {
            {componentTemplate->getTemplate(), overrideInfo.overriddenTemplate->getTemplate()}
        };
        replaceObjectReferences(this, replacements, EObjectTraversalMode::EntireObjectTree);
    }
    markDirty(this);
}

void ActorPrefab::clearComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences)
{
    ObjectTemplate *revertToCompTemplate = getTemplateToOverride(overrideInfo);
    ActorPrefab *revertToCompPrefab = prefabFromCompTemplate(revertToCompTemplate);
    TransformComponent *revertToTfComponent = revertToCompTemplate->getTemplateAs<TransformComponent>();

    if (revertToTfComponent)
    {
        TransformComponent *revertingComp = static_cast<TransformComponent *>(overrideInfo.overriddenTemplate->getTemplate());
        if (revertingComp == rootComponent)
        {
            // If parent root component is same as reverting to component we do not have to override root component at all
            if (parentPrefab->getRootComponent() == revertToTfComponent)
            {
                rootComponent = nullptr;
            }
            else
            {
                rootComponent = revertToTfComponent;
            }
        }
        else
        {
            auto tfAttachedToItr = componentAttachedTo.find(revertingComp);
            debugAssert(tfAttachedToItr != componentAttachedTo.end());
            setComponentAttachedTo(revertToTfComponent, tfAttachedToItr->second);
            setComponentAttachedTo(revertingComp, nullptr);
        }

        // Replace all the overrides with previous override or component
        for (const std::pair<TransformComponent *const, TransformComponent *> &attachedToPair : componentAttachedTo)
        {
            if (attachedToPair.second == revertingComp)
            {
                // Map will not change since only value will be changed
                setComponentAttachedTo(attachedToPair.first, revertToTfComponent);
            }
        }
    }

    if (bReplaceReferences)
    {
        std::unordered_map<Object *, Object *> replacements = {
            {overrideInfo.overriddenTemplate->getTemplate(), revertToCompTemplate->getTemplate()}
        };
        replaceObjectReferences(this, replacements, EObjectTraversalMode::EntireObjectTree);
    }
    markDirty(this);

    overrideInfo.overriddenTemplate->beginDestroy();
    overrideInfo.overriddenTemplate = nullptr;
}

FORCE_INLINE void ActorPrefab::postAddComponent(Object *comp)
{
    if (TransformComponent *tfComp = cast<TransformComponent>(comp))
    {
        setComponentAttachedTo(tfComp, getRootComponent());
    }
    markDirty(this);
}

//////////////////////////////////////////////////////////////////////////
// Components impl
//////////////////////////////////////////////////////////////////////////

Actor *LogicComponent::getActor() const
{
    if (Actor *actor = cast<Actor>(getOuter()))
    {
        return actor;
    }
    // If stored inside prefab, Template will be subobject of Actor itself
    else if (ObjectTemplate *objTemplate = cast<ObjectTemplate>(getOuter()))
    {
        return cast<Actor>(objTemplate->getOuter());
    }
    return nullptr;
}

Actor *TransformComponent::getActor() const
{
    if (Actor *actor = cast<Actor>(getOuter()))
    {
        return actor;
    }
    // If stored inside prefab, Template will be subobject of Actor itself
    else if (ObjectTemplate *objTemplate = cast<ObjectTemplate>(getOuter()))
    {
        return cast<Actor>(objTemplate->getOuter());
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
/// Actor impl
//////////////////////////////////////////////////////////////////////////

World *Actor::getWorld() const
{
    if (World *world = cast<World>(getOuter()))
    {
        return world;
    }
    // We mostly store the actor as prefab inside world unless it is spawned from a class at runtime or never modified
    else if (ObjectTemplate *objTemplate = cast<ObjectTemplate>(getOuter()))
    {
        return cast<World>(objTemplate->getOuter()->getOuter());
    }
    return nullptr;
}

void Actor::addComponent(Object *component)
{
    if (PropertyHelper::isChildOf<TransformComponent>(component->getType()))
    {
        TransformComponent *tfComp = static_cast<TransformComponent *>(component);
        transformComps.insert(tfComp);
        if (tfComp->getAttachedTo() == nullptr)
        {
            tfComp->setAttachedTo(rootComponent);
        }
        if (World *world = getWorld())
        {
            world->onComponentAdded(this, tfComp);
        }
    }
    else if (PropertyHelper::isChildOf<LogicComponent>(component->getType()))
    {
        logicComps.insert(static_cast<LogicComponent *>(component));
    }
}

void Actor::removeComponent(Object *component)
{
    if (PropertyHelper::isChildOf<TransformComponent>(component->getType()))
    {
        TransformComponent *tfComp = static_cast<TransformComponent *>(component);
        if (transformComps.erase(tfComp) != 0)
        {
            if (tfComp == rootComponent)
            {
                if (World *world = getWorld())
                {
                    std::vector<TransformComponent *> attachments;
                    getWorld()->componentsAttachedTo(attachments, tfComp);
                    // Just set the first child component of root as new root the world will fix up attachments
                    for (TransformComponent *attachment : attachments)
                    {
                        if (attachment->getActor() == this)
                        {
                            rootComponent = attachment;
                            break;
                        }
                    }
                    // This will reattach  all components to new root that was attached to old root
                    getWorld()->onComponentRemoved(this, tfComp);
                }
                else
                {
                    // This happens if actor is not created from world. Instead may be created as part of ActorPrefab
                    for (TransformComponent *attachment : transformComps)
                    {
                        if (attachment->getAttachedTo() == tfComp)
                        {
                            rootComponent = attachment;
                            attachment->setAttachedTo(nullptr);
                            break;
                        }
                    }
                    for (TransformComponent *attachment : transformComps)
                    {
                        if (attachment->getAttachedTo() == tfComp)
                        {
                            attachment->setAttachedTo(rootComponent);
                        }
                    }
                }
            }
        }
    }
    else if (PropertyHelper::isChildOf<LogicComponent>(component->getType()))
    {
        logicComps.erase(static_cast<LogicComponent *>(component));
    }
}

void Actor::attachActor(TransformComponent *otherComponent)
{
    debugAssert(rootComponent);
    rootComponent->attachComponent(otherComponent);
}

void Actor::detachActor() { rootComponent->detachComponent(); }

cbe::Object *Actor::componentFromClass(CBEClass clazz, const TChar *componentName, EObjectFlags flags)
{
    Object *comp = create(clazz, componentName, this, flags);
    return comp;
}

Object *Actor::componentFromTemplate(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags flags)
{
    Object *comp = create(objTemplate, componentName, this, flags);
    return comp;
}

} // namespace cbe
