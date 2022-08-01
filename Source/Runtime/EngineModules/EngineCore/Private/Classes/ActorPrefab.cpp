/*!
 * \file ActorPrefab.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Classes/ActorPrefab.h"
#include "CBEObjectHelpers.h"
#include "Classes/Actor.h"

namespace cbe
{

bool ActorPrefab::canOverrideRootComp() const
{
    const ActorPrefab *overriddenAt = this;
    // At least the first prefab from native class will have root component overridden if native class has no root component
    while (overriddenAt)
    {
        if (overriddenAt->rootComponent)
        {
            return true;
        }
        overriddenAt = overriddenAt->parentPrefab;
    }
    return false;
}

Object *ActorPrefab::onComponentFieldModify(const FieldProperty *prop, Object *obj)
{
    debugAssert(obj && prop);
    Object *comp = modifyComponent(obj);
    ObjectTemplate *objTemplate = objectTemplateFromObj(comp);
    objTemplate->onFieldModified(prop, obj);
    return comp;
}

Object *ActorPrefab::onComponentFieldReset(const FieldProperty *prop, Object *obj)
{
    debugAssert(obj && prop);
    Object *comp = modifyComponent(obj);
    ObjectTemplate *objTemplate = objectTemplateFromObj(comp);
    objTemplate->onFieldReset(prop, obj);
    return comp;
}

void ActorPrefab::onActorFieldModify(const FieldProperty *prop, Actor *actor)
{
    debugAssert(actor && actorTemplate->getTemplate() == actor && prop);
    actorTemplate->onFieldModified(prop, actor);
    markDirty(this);
}

void ActorPrefab::onActorFieldReset(const FieldProperty *prop, Actor *actor)
{
    debugAssert(actor && actorTemplate->getTemplate() == actor && prop);
    actorTemplate->onFieldReset(prop, actor);
    markDirty(this);
}

ActorPrefab *ActorPrefab::prefabFromActorTemplate(ObjectTemplate *actorTemplate)
{
    // Outer of actor template must be actor prefab
    return actorTemplate ? cast<ActorPrefab>(actorTemplate->getOuter()) : nullptr;
}

ActorPrefab *ActorPrefab::prefabFromCompTemplate(ObjectTemplate *compTemplate)
{
    // Outer of component template must be the actor from actor template, Look up Component::getActor()
    return compTemplate ? cast<ActorPrefab>(compTemplate->getOuter()->getOuter()->getOuter()) : nullptr;
}

ObjectTemplate *ActorPrefab::objectTemplateFromObj(Object *obj) { return obj ? cast<ObjectTemplate>(obj->getOuter()) : nullptr; }

FORCE_INLINE bool ActorPrefab::isOwnedComponent(Object *comp) const { return prefabFromCompTemplate(objectTemplateFromObj(comp)) == this; }

} // namespace cbe
