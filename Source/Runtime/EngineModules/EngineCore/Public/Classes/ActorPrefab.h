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

// Included to support generated codes
#include "ObjectTemplate.h"
#ifdef __REF_PARSE__
#include "Components/ComponentBase.h"
#endif

#include "ActorPrefab.gen.h"

namespace cbe
{
class ObjectTemplate;
class TransformComponent;
class Actor;

class META_ANNOTATE_API(ENGINECORE_EXPORT) ActorPrefab : public Object
{
    GENERATED_CODES()

private:
    struct META_ANNOTATE() ComponentOverrideInfo
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
    };

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
    // will not contains actor entry or not overridden components
    META_ANNOTATE()
    std::map<TransformComponent *, TransformComponent *> componentAttachedTo;

public:
    ActorPrefab()
        : actorClass(nullptr)
        , parentPrefab(nullptr)
        , actorTemplate(nullptr)
        , rootComponent(nullptr)
    {}

    ActorPrefab(StringID className, const String &actorName);
    ActorPrefab(ActorPrefab *inPrefab, const String &name);

    bool canOverrideRootComp() const;
    /**
     * Informs ActorPrefab that this component need modification. ActorPrefab decides if component needs override
     * If override is needed creates the override and returns the overridden component
     */
    Object *modifyComponent(Object *modifyingComp);
    // Just few helpers that composes and adds to modifyComponent(), Must be called before modifying for component. Actors always has overrides
    Object *onComponentFieldModify(const FieldProperty *prop, Object *obj);
    Object *onComponentFieldReset(const FieldProperty *prop, Object *obj);
    void onActorFieldModify(const FieldProperty *prop, Actor *actor);
    void onActorFieldReset(const FieldProperty *prop, Actor *actor);

    void setRootComponent(TransformComponent *component);
    void setComponentAttachedTo(TransformComponent *attachingComp, TransformComponent *attachedToComp);

    /**
     * Create and add a component to prefab and if it is a TransformComponent attaches it to root component
     * Returns the created component
     */
    Object *addComponent(CBEClass compClass, const String &compName);
    Object *addComponent(ObjectTemplate *compTemplate, const String &compName);
    void removeComponent(Object *comp);

    /* cbe::Object overrides */
    ObjectArchive &serialize(ObjectArchive &ar) override;
    /* Overrides ends */
private:
    static ActorPrefab *prefabFromActorTemplate(ObjectTemplate *actorTemplate);
    static ActorPrefab *prefabFromCompTemplate(ObjectTemplate *compTemplate);
    static ObjectTemplate *objectTemplateFromObj(Object *obj);
    FORCE_INLINE static ObjectTemplate *getTemplateToOverride(const ComponentOverrideInfo &overrideInfo)
    {
        return overrideInfo.lastOverride ? overrideInfo.lastOverride : overrideInfo.baseTemplate;
    }

    FORCE_INLINE bool isNativeComponent(Object *obj) const;
    // If this prefab owns this component
    FORCE_INLINE bool isOwnedComponent(Object *comp) const;
    TransformComponent *getRootComponent() const;

    void createComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences);
    void clearComponentOverride(ComponentOverrideInfo &overrideInfo, bool bReplaceReferences);
    FORCE_INLINE void postAddComponent(Object *comp);
};

} // namespace cbe