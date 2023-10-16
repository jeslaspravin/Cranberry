/*!
 * \file ActorPrefab.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Classes/ActorPrefab.h"
#include "CBEObjectHelpers.h"
#include "Classes/Actor.h"
#include "ObjectTemplate.h"

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

Actor *ActorPrefab::getActorTemplate() const { return actorTemplate->getTemplateAs<Actor>(); }

ActorPrefab *ActorPrefab::prefabFromActorTemplate(const ObjectTemplate *actorTemplate)
{
    // Outer of actor template must be actor prefab
    return actorTemplate ? cast<ActorPrefab>(actorTemplate->getOuter()) : nullptr;
}

ActorPrefab *ActorPrefab::prefabFromCompTemplate(const ObjectTemplate *compTemplate)
{
    // Outer of component template must be the actor template, Look up Component::getActor()
    return compTemplate ? cast<ActorPrefab>(compTemplate->getOuter()->getOuter()) : nullptr;
}

ObjectTemplate *ActorPrefab::objectTemplateFromObj(const Object *obj) { return obj ? cast<ObjectTemplate>(obj->getOuter()) : nullptr; }

ObjectTemplate *ActorPrefab::objectTemplateFromNativeComp(const Object *obj)
{
    return obj && isNativeComponent(obj) ? cast<ObjectTemplate>(obj->getOuter()->getOuter()) : nullptr;
}

bool ActorPrefab::isOwnedComponent(const Object *comp) const { return prefabFromCompTemplate(objectTemplateFromObj(comp)) == this; }

} // namespace cbe
