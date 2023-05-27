/*!
 * \file ActorPrefab.h
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

#include "ActorPrefab.gen.h"

namespace cbe
{
class ObjectTemplate;
class TransformComponent;
class TransformLeafComponent;
class Actor;
class World;

class ENGINECORE_EXPORT ActorPrefab : public Object
{
    GENERATED_CODES()

public:
    struct ComponentOverrideInfo
    {
        GENERATED_CODES()

    public:
        // Base template is component template of prefab in which it is first created
        META_ANNOTATE()
        ObjectTemplate *baseTemplate;
        // Last template that overrides the base template excluding overrides from this ActorPrefab. This is the first override of base template
        // if null
        META_ANNOTATE()
        ObjectTemplate *lastOverride = nullptr;
        // If not null it means that this Prefab overrides last override/base template.
        // However since I use ActorPrefab directly in world this will always be generated. At least until world recreates actors from prefab
        META_ANNOTATE()
        ObjectTemplate *overriddenTemplate = nullptr;
    } META_ANNOTATE();

private:
    // Will never be null
    CBEClass actorClass;
    // Will be null if created from class instead of another prefab
    META_ANNOTATE()
    ActorPrefab *parentPrefab;

    META_ANNOTATE()
    ObjectTemplate *actorTemplate;
    // Components that are created in this prefab
    META_ANNOTATE()
    std::vector<ObjectTemplate *> components;
    // List of components from its parent prefabs, That can either be overridden
    META_ANNOTATE()
    std::vector<ComponentOverrideInfo> componentOverrides;

    // root component override
    META_ANNOTATE()
    TransformComponent *rootComponent;
    // will not contains actor entry
    META_ANNOTATE()
    std::unordered_map<TransformComponent *, TransformComponent *> componentAttachedTo;

public:
    ActorPrefab()
        : actorClass(nullptr)
        , parentPrefab(nullptr)
        , actorTemplate(nullptr)
        , rootComponent(nullptr)
    {}

    ActorPrefab(StringID className, String actorName);
    ActorPrefab(ActorPrefab *inPrefab, String name);

    bool canOverrideRootComp() const;
    /**
     * Informs ActorPrefab that this component need modification. ActorPrefab decides if component needs override
     * If override is needed creates the override and returns the overridden component
     * This also triggers modify on components that references this component
     */
    Object *modifyComponent(Object *modifyingComp);
    void onActorFieldModify(const FieldProperty *prop, Actor *actor);
    void onActorFieldReset(const FieldProperty *prop, Actor *actor);
    /**
     * copyFrom assumes that both prefab is from same parent and is up to date with the parent.
     * It cannot be used to update parent changes to itself
     */
    bool copyFrom(ActorPrefab *otherPrefab);
    FORCE_INLINE bool copyCompatible(ActorPrefab *otherPrefab) const
    {
        return otherPrefab->actorClass == actorClass && otherPrefab->parentPrefab == parentPrefab;
    }

    // Root component can only be set if there is no native root component or we are editing a prefab
    void setRootComponent(TransformComponent *component);
    void setComponentAttachedTo(TransformComponent *attachingComp, TransformComponent *attachedToComp);
    void setLeafAttachedTo(TransformLeafComponent *attachingComp, TransformComponent *attachedToComp);

    /**
     * Create and add a component to prefab and if it is a TransformComponent attaches it to root component
     * Returns the created component
     */
    Object *addComponent(CBEClass compClass, StringView compName);
    Object *addComponent(ObjectTemplate *compTemplate, StringView compName);
    void removeComponent(Object *component);

    FORCE_INLINE CBEClass getActorClass() const { return actorClass; }
    FORCE_INLINE ActorPrefab *getParentPrefab() const { return parentPrefab; }
    const std::vector<ObjectTemplate *> &getPrefabComponents() const { return components; }
    const std::vector<ComponentOverrideInfo> &getOverridenComponents() const { return componentOverrides; }
    Actor *getActorTemplate() const;
    World *getWorld() const;
    // Use below getRootComponent if you want to find the root component from prefab that might not be a part of Prepared to play world
    TransformComponent *getRootComponent() const;
    /**
     * Provides attachment information available in this object. Does not do deeper scan.
     * To get the actual attachment information based on the context use WACHelpers function
     * Use below functions if you are sure what you are accessing
     */
    TransformComponent *getAttachedToComp(const TransformComponent *component) const;
    void getCompAttaches(const TransformComponent *component, std::vector<TransformComponent *> tfComps) const;
    void getCompAttaches(const TransformComponent *component, std::vector<TransformLeafComponent *> leafComps) const;

    /* cbe::Object overrides */
    ObjectArchive &serialize(ObjectArchive &ar) override;
    void onPostSerialize(const ObjectArchive &ar) override;
    /* Overrides ends */

    // Helper functions
    static ActorPrefab *prefabFromActorTemplate(const ObjectTemplate *actorTemplate);
    static ActorPrefab *prefabFromCompTemplate(const ObjectTemplate *compTemplate);
    static ActorPrefab *prefabFromComponent(const Object *component)
    {
        return isNativeComponent(component) ? prefabFromActorTemplate(objectTemplateFromNativeComp(component))
                                            : prefabFromCompTemplate(objectTemplateFromObj(component));
    }
    // Object's template for native components there is no immediate templates and this function returns null in that case
    static ObjectTemplate *objectTemplateFromObj(const Object *obj);
    // Returns Actor's object template which will hold the native component's template as well
    static ObjectTemplate *objectTemplateFromNativeComp(const Object *obj);
    // initializes world actor prefab by connecting all attachments
    static void initializeActor(ActorPrefab *inPrefab);

    static bool isNativeComponent(const Object *obj);
    // If this prefab owns this component
    bool isOwnedComponent(const Object *comp) const;

private:
    FORCE_INLINE static ObjectTemplate *getTemplateToOverride(const ComponentOverrideInfo &overrideInfo)
    {
        return overrideInfo.lastOverride ? overrideInfo.lastOverride : overrideInfo.baseTemplate;
    }

    void createComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences);
    void clearComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences);
    FORCE_INLINE void postAddComponent(Object *comp);

} META_ANNOTATE(NoExport);

} // namespace cbe