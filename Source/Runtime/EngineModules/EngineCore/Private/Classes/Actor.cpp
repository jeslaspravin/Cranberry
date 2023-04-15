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
#include "Components/ComponentBase.h"

namespace cbe
{

//////////////////////////////////////////////////////////////////////////
// ActorPrefab impl
//////////////////////////////////////////////////////////////////////////

constexpr inline const uint32 ACTOR_PREFAB_SERIALIZER_VERSION = 0;
constexpr inline const uint32 ACTOR_PREFAB_SERIALIZER_CUTOFF_VERSION = 0;
STRINGID_CONSTEXPR inline const StringID ACTOR_PREFAB_CUSTOM_VERSION_ID = STRID("ActorPrefabSerializer");

ActorPrefab::ActorPrefab(StringID className, const String &actorName)
    : parentPrefab(nullptr)
{
    ObjectPrivateDataView thisObjDatV = getObjectData();

    actorClass = IReflectionRuntimeModule::get()->getClassType(className);
    actorTemplate
        = create<ObjectTemplate, StringID, const String &>(actorName + TCHAR("_AcTmpt"), this, thisObjDatV.flags, className, actorName);

    // If there is no root component already then we must create a root component and add it
    if (!static_cast<Actor *>(actorTemplate->getTemplate())->getRootComponent())
    {
        String componentName = TCHAR("RootComp");
        String compTemplateName = TCHAR("RootComp_CpTmpt");
        components.emplace_back(create<ObjectTemplate, StringID, const String &>(
            compTemplateName, actorTemplate, thisObjDatV.flags, TransformComponent::staticType()->name, componentName
        ));
        rootComponent = static_cast<TransformComponent *>(components.back()->getTemplate());
    }
    markDirty(this);
}

ActorPrefab::ActorPrefab(ActorPrefab *inPrefab, const String &name)
    : parentPrefab(inPrefab)
{
    debugAssert(parentPrefab);
    ObjectPrivateDataView thisObjDatV = getObjectData();

    actorClass = parentPrefab->actorClass;

    String actorTemplateName = name + TCHAR("_AcTmpt");
    actorTemplate = create<ObjectTemplate, ObjectTemplate *, const String &>(
        actorTemplateName, this, thisObjDatV.flags, parentPrefab->actorTemplate, name
    );
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
            String attachedToPath = ObjectPathHelper::getFullPath(compAttachedTo.second->getObjectData().name, actorTemplate->getTemplate());
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

bool ActorPrefab::copyFrom(ActorPrefab *otherPrefab)
{
    if (!copyCompatible(otherPrefab))
    {
        return false;
    }
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();

    bool bCopiedActorTemplate = actorTemplate->copyFrom(otherPrefab->actorTemplate);
    if (!bCopiedActorTemplate)
    {
        LOG_ERROR(
            "ActorPrefab", "Cannot copy mismatched actor templates[To: {}, From: {}]", getObjectData().path, otherPrefab->getObjectData().path
        );
        return false;
    }

    // For setting up new root in this prefab
    TransformComponent *otherPrefabRoot = otherPrefab->getRootComponent();

    // Copy/Add all prefab created components first attachments will be handled at last
    // First have to create all not available components so that copy can reference it later
    std::unordered_set<ObjectTemplate *> compsToRemove(components.cbegin(), components.cend());
    for (ObjectTemplate *otherComp : otherPrefab->components)
    {
        ObjectPrivateDataView otherCompDatV = objectsDb.getObjectData(otherComp->getDbIdx());
        ObjectPrivateDataView otherCompTemplateDatV = objectsDb.getObjectData(otherComp->getTemplate()->getDbIdx());

        ObjectTemplate *thisComp = get<ObjectTemplate>(ObjectPathHelper::getFullPath(otherCompDatV.name, actorTemplate).getChar());
        if (thisComp == nullptr)
        {
            if (ObjectTemplate *parentTemplate = otherComp->getParentTemplate())
            {
                thisComp = objectTemplateFromObj(addComponent(parentTemplate, otherCompTemplateDatV.name));
            }
            else
            {
                thisComp = objectTemplateFromObj(addComponent(otherComp->getTemplateClass(), otherCompTemplateDatV.name));
            }
            debugAssert(thisComp);
        }
        else
        {
            debugAssert(compsToRemove.contains(thisComp));
        }
    }
    for (ObjectTemplate *otherComp : otherPrefab->components)
    {
        ObjectPrivateDataView otherCompDatV = objectsDb.getObjectData(otherComp->getDbIdx());

        ObjectTemplate *thisComp = get<ObjectTemplate>(ObjectPathHelper::getFullPath(otherCompDatV.name, actorTemplate).getChar());
        bool bIsCopied = thisComp->copyFrom(otherComp);
        compsToRemove.erase(thisComp);
        if (!bIsCopied)
        {
            LOG_ERROR(
                "ActorPrefab", "Failed to copy component templates[To: {}, From: {}]", thisComp->getObjectData().path, otherCompDatV.path
            );
            return false;
        }

        // Setup new root component
        if (otherPrefabRoot == otherComp->getTemplateAs<TransformComponent>())
        {
            setRootComponent(thisComp->getTemplateAs<TransformComponent>());
        }
    }

    // Comp or add overridden components, In overridden case there is no delete components
    // First create overrides that does not exists
    for (const ComponentOverrideInfo &otherOverrides : otherPrefab->componentOverrides)
    {
        auto compOverrideItr = std::find_if(
            componentOverrides.begin(), componentOverrides.end(),
            [&otherOverrides](const ComponentOverrideInfo &overrideInfo)
            {
                return getTemplateToOverride(overrideInfo) == getTemplateToOverride(otherOverrides);
            }
        );

        if (compOverrideItr == componentOverrides.end())
        {
            LOG_ERROR(
                "ActorPrefab", "Cannot find component override entry for {}", getTemplateToOverride(otherOverrides)->getObjectData().path
            );
            return false;
        }

        ComponentOverrideInfo &thisCompOverride = *compOverrideItr;
        if (thisCompOverride.overriddenTemplate == nullptr && otherOverrides.overriddenTemplate != nullptr)
        {
            modifyComponent(getTemplateToOverride(thisCompOverride)->getTemplate());
            // Modify component would have populated thisCompOverride
            debugAssert(thisCompOverride.overriddenTemplate);
        }
    }
    for (const ComponentOverrideInfo &otherOverrides : otherPrefab->componentOverrides)
    {
        auto compOverrideItr = std::find_if(
            componentOverrides.begin(), componentOverrides.end(),
            [&otherOverrides](const ComponentOverrideInfo &overrideInfo)
            {
                return getTemplateToOverride(overrideInfo) == getTemplateToOverride(otherOverrides);
            }
        );

        ComponentOverrideInfo &thisCompOverride = *compOverrideItr;
        if (otherOverrides.overriddenTemplate == nullptr)
        {
            if (thisCompOverride.overriddenTemplate != nullptr)
            {
                LOG_WARN("ActorPrefab", "Removing overridden component when all modified field is reset is not supported yet!");
            }
            continue;
        }

        thisCompOverride.overriddenTemplate->copyFrom(otherOverrides.overriddenTemplate);
        // Setup new root component
        if (otherPrefabRoot == otherOverrides.overriddenTemplate->getTemplateAs<TransformComponent>())
        {
            setRootComponent(thisCompOverride.overriddenTemplate->getTemplateAs<TransformComponent>());
        }
    }

    // Remove this prefab's components
    for (ObjectTemplate *compTemplate : compsToRemove)
    {
        removeComponent(compTemplate->getTemplate());
    }

    componentAttachedTo.clear();
    componentAttachedTo.reserve(otherPrefab->componentAttachedTo.size());
    for (const std::pair<TransformComponent *const, TransformComponent *> &otherAttachedPair : otherPrefab->componentAttachedTo)
    {
        debugAssert(otherAttachedPair.first && otherAttachedPair.second);

        TransformComponent *attachingComp = otherAttachedPair.first;
        TransformComponent *attachedToComp = otherAttachedPair.second;
        if (otherPrefab->isOwnedComponent(attachingComp))
        {
            attachingComp
                = get<TransformComponent>(ObjectPathHelper::getFullPath(attachingComp->getObjectData().name, actorTemplate).getChar());
        }
        if (otherPrefab->isOwnedComponent(attachedToComp))
        {
            attachedToComp
                = get<TransformComponent>(ObjectPathHelper::getFullPath(attachedToComp->getObjectData().name, actorTemplate).getChar());
        }
        // Below assert must not trigger as above component and overrides copy logics must create all necessary components
        fatalAssert(attachingComp && attachedToComp);

        setComponentAttachedTo(attachingComp, attachedToComp);
    }
    markDirty(this);
    return true;
}

void ActorPrefab::setRootComponent(TransformComponent *component)
{
    if (!isOwnedComponent(component))
    {
        LOG_WARN("ActorPrefab", "Component is not owned, Call modifyComponent() to override the component!", component->getObjectData().path);
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
            "ActorPrefab", "Attaching component is not owned, Call modifyComponent() to override the component!",
            attachingComp->getObjectData().path
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
    ObjectPrivateDataView thisObjDatV = getObjectData();

    String compTemplateName = compName + TCHAR("_CpTmpt");
    ObjectTemplate *compTemplate
        = create<ObjectTemplate, StringID, const String &>(compTemplateName, actorTemplate, thisObjDatV.flags, compClass->name, compName);
    components.emplace_back(compTemplate);
    postAddComponent(compTemplate->getTemplate());
    return compTemplate->getTemplate();
}

Object *ActorPrefab::addComponent(ObjectTemplate *compTemplate, const String &compName)
{
    ObjectPrivateDataView thisObjDatV = getObjectData();

    String compTemplateName = compName + TCHAR("_CpTmpt");
    ObjectTemplate *compObjTemplate
        = create<ObjectTemplate, ObjectTemplate *, const String &>(compTemplateName, actorTemplate, thisObjDatV.flags, compTemplate, compName);
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
        LOG("ActorPrefab", "Component {} is already removed", comp->getObjectData().name);
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
    components.erase(compTemplateItr);
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

TransformComponent *ActorPrefab::getAttachedToComp(const TransformComponent *component) const
{
    debugAssert(isValidFast(component) && componentAttachedTo.contains(component));
    return componentAttachedTo.find(component)->second;
}

ObjectArchive &ActorPrefab::serialize(ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        uint32 dataVersion = ar.getCustomVersion(uint32(ACTOR_PREFAB_CUSTOM_VERSION_ID));
        // This must crash
        fatalAssertf(
            ACTOR_PREFAB_SERIALIZER_CUTOFF_VERSION >= dataVersion,
            "Version of ActorPrefab {} loaded from package {} is outdated, Minimum supported {}!", dataVersion, getObjectData().path,
            ACTOR_PREFAB_SERIALIZER_CUTOFF_VERSION
        );
    }
    else
    {
        ar.setCustomVersion(uint32(ACTOR_PREFAB_CUSTOM_VERSION_ID), ACTOR_PREFAB_SERIALIZER_VERSION);
    }

    ar << actorClass;
    return ObjectSerializationHelpers::serializeAllFields(this, ar);
}

void ActorPrefab::onPostSerialize(const ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        if (parentPrefab == nullptr)
        {
            componentOverrides.clear();
        }
        alertAlwaysf(actorClass && actorTemplate, "Missing actor class/Template for actor prefab {}", getObjectData().path);
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
            if (compAttachedToPair.second != nullptr && !isValidFast(compAttachedToPair.second))
            {
                // walk the tree and find first valid attachable component
                auto nextAttachedToItr = componentAttachedTo.find(compAttachedToPair.second);
                while (nextAttachedToItr != componentAttachedTo.end())
                {
                    // If reached nullptr then we lost the chain leaving the loop is better
                    if (nextAttachedToItr->second == nullptr || isValidFast(nextAttachedToItr->second))
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
            if (tfComp != actor->rootComponent)
            {
                auto attachedToItr = inPrefab->componentAttachedTo.find(tfComp);
                alertAlways(attachedToItr != inPrefab->componentAttachedTo.end());
                if (attachedToItr->second == tfComp->getAttachedTo())
                {
                    tfComp->setAttachedTo(attachedToItr->second);
                }
            }
            actor->transformComps.insert(tfComp);
        }
        else if (LogicComponent *logicComp = cast<LogicComponent>(comp))
        {
            actor->logicComps.insert(logicComp);
        }
        else
        {
            fatalAssertf(
                false, "Why?? Component {} of type {} is not a valid component", comp->getObjectData().name, comp->getType()->nameString
            );
        }
    };
    for (ObjectTemplate *comp : inPrefab->components)
    {
        addCompToActor(comp->getTemplate());
    }
    for (const ComponentOverrideInfo &overrideInfo : inPrefab->componentOverrides)
    {
        debugAssertf(overrideInfo.overriddenTemplate, "World's ActorPrefab must have all of its component overridden!");
        addCompToActor(overrideInfo.overriddenTemplate->getTemplate());
    }
    debugAssert(
        actor->rootComponent
        && (actor->logicComps.size() + actor->transformComps.size())
               == (inPrefab->components.size() + inPrefab->componentOverrides.size() + nativeCompsCount)
    );
}

bool ActorPrefab::isNativeComponent(Object *obj) const { return obj && PropertyHelper::isChildOf<Actor>(obj->getOuter()->getType()); }

void ActorPrefab::createComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences)
{
    const CoreObjectsDB &objsDb = ICoreObjectsModule::get()->getObjectsDB();

    ObjectTemplate *compTemplateObj = getTemplateToOverride(overrideInfo);
    TransformComponent *tfComponent = compTemplateObj->getTemplateAs<TransformComponent>();

#if DEBUG_VALIDATIONS
    ActorPrefab *actorPrefab = prefabFromCompTemplate(compTemplateObj);
    LogicComponent *logicComponent = compTemplateObj->getTemplateAs<LogicComponent>();
    debugAssert(compTemplateObj && (logicComponent || tfComponent) && actorPrefab && actorPrefab != this);
#endif
    ObjectPrivateDataView compTemplateObjDatV = objsDb.getObjectData(compTemplateObj->getDbIdx());
    ObjectPrivateDataView compTemplateDatV = objsDb.getObjectData(compTemplateObj->getTemplate()->getDbIdx());

    overrideInfo.overriddenTemplate = create<ObjectTemplate, ObjectTemplate *, const String &>(
        compTemplateObjDatV.name, actorTemplate, compTemplateObjDatV.flags, compTemplateObj, compTemplateDatV.name
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
            {compTemplateObj->getTemplate(), overrideInfo.overriddenTemplate->getTemplate()}
        };
        replaceObjectReferences(this, replacements, EObjectTraversalMode::EntireObjectTree);
    }
    markDirty(this);
}

void ActorPrefab::clearComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences)
{
    ObjectTemplate *revertToCompTemplate = getTemplateToOverride(overrideInfo);
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
    // If stored inside prefab, Template will be subobject of Actor template itself
    else if (ObjectTemplate *objTemplate = ActorPrefab::objectTemplateFromObj(this))
    {
        ActorPrefab *prefab = ActorPrefab::prefabFromCompTemplate(objTemplate);
        debugAssert(prefab);
        return prefab->getActorTemplate();
    }
    return nullptr;
}

cbe::TransformComponent *TransformComponent::canonicalAttachedTo()
{
    // Will be null when not playing or when not attached to anything
    if (attachedTo == nullptr)
    {
        ObjectTemplate *objTemplate = ActorPrefab::objectTemplateFromObj(this);
        ActorPrefab *prefab = ActorPrefab::prefabFromCompTemplate(objTemplate);
        World *world = getWorld();
        if (objTemplate && prefab)
        {
            TransformComponent *attachedToComp = nullptr;
            if (prefab->getRootComponent() != this)
            {
                attachedToComp = prefab->getAttachedToComp(this);
            }
            else if (world && !EWorldState::isPlayState(world->getState())) // If root component then world will have its attachment information
            {
                attachedToComp = world->getActorAttachedToComp(getActor());
            }
            return attachedToComp;
        }
    }
    else
    {
        return attachedTo;
    }
    return nullptr;
}

Transform3D TransformComponent::getWorldTransform() const
{
    World *world = getWorld();
    if (world && world->hasWorldTf(this))
    {
        return world->getWorldTf(this);
    }
    else
    {
        ObjectTemplate *objTemplate = ActorPrefab::objectTemplateFromObj(this);
        ActorPrefab *prefab = ActorPrefab::prefabFromCompTemplate(objTemplate);
        if (objTemplate && prefab)
        {
            TransformComponent *attachedToComp = nullptr;
            if (prefab->getRootComponent() != this)
            {
                attachedToComp = prefab->getAttachedToComp(this);
            }
            else if (world && !EWorldState::isPlayState(world->getState()))
            {
                attachedToComp = world->getActorAttachedToComp(getActor());
            }

            if (attachedToComp)
            {
                return attachedToComp->getWorldTransform().transform(relativeTf);
            }
        }
    }
    return relativeTf;
}

Actor *TransformComponent::getActor() const
{
    if (Actor *actor = cast<Actor>(getOuter()))
    {
        return actor;
    }
    // If stored inside prefab, Template will be subobject of Actor template itself
    else if (ObjectTemplate *objTemplate = ActorPrefab::objectTemplateFromObj(this))
    {
        ActorPrefab *prefab = ActorPrefab::prefabFromCompTemplate(objTemplate);
        debugAssert(prefab);
        return prefab->getActorTemplate();
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
            world->tfComponentAdded(this, tfComp);
        }
    }
    else if (PropertyHelper::isChildOf<LogicComponent>(component->getType()))
    {
        logicComps.insert(static_cast<LogicComponent *>(component));
        if (World *world = getWorld())
        {
            world->logicComponentAdded(this, static_cast<LogicComponent *>(component));
        }
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
                    getWorld()->getComponentsAttachedTo(attachments, tfComp);
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
                    getWorld()->tfComponentRemoved(this, tfComp);
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
        if (World *world = getWorld())
        {
            world->logicComponentRemoved(this, static_cast<LogicComponent *>(component));
        }
    }
}

void Actor::attachActor(TransformComponent *otherComponent)
{
    debugAssert(rootComponent);
    rootComponent->attachComponent(otherComponent);
}

cbe::Actor *Actor::getActorAttachedTo() const
{
    if (rootComponent && rootComponent->getAttachedTo())
    {
        return rootComponent->getAttachedTo()->getActor();
    }
    return nullptr;
}

void Actor::detachActor() { rootComponent->detachComponent(); }

cbe::Object *Actor::componentFromClass(CBEClass clazz, const TChar *componentName, EObjectFlags componentFlags)
{
    Object *comp = create(clazz, componentName, this, componentFlags);
    return comp;
}

Object *Actor::componentFromTemplate(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags componentFlags)
{
    Object *comp = create(objTemplate, componentName, this, componentFlags);
    return comp;
}

} // namespace cbe
