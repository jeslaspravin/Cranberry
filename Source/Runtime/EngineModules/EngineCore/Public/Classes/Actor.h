/*!
 * \file Actor.h
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

// Including in reflection parser alone to detect types as reflected, This is necessary for validation
#ifdef __REF_PARSE__
#include "Components/ComponentBase.h"
#endif
#include "Actor.gen.h"

namespace cbe
{
class World;
class LogicComponent;
class TransformComponent;
class ObjectTemplate;
class ActorPrefab;

class ENGINECORE_EXPORT Actor : public Object
{
    GENERATED_CODES()
private:
    friend ActorPrefab;

    META_ANNOTATE()
    std::set<TransformComponent *> transformComps;

    META_ANNOTATE()
    std::set<LogicComponent *> logicComps;

    META_ANNOTATE()
    TransformComponent *rootComponent;

public:
    // For any world spawned actor, its outer will be world itself
    World *getWorld() const;
    FORCE_INLINE TransformComponent *getRootComponent() const { return rootComponent; }
    FORCE_INLINE const std::set<TransformComponent *> &getTransformComponents() const { return transformComps; }
    FORCE_INLINE const std::set<LogicComponent *> &getLogicComponents() const { return logicComps; }

    void addComponent(Object *component);
    void removeComponent(Object *component);

    void attachActor(TransformComponent *otherComponent);
    Actor *getActorAttachedTo() const;
    void detachActor();

    // Helpers
    template <typename T>
    FORCE_INLINE T *createComponent(const TChar *componentName, EObjectFlags flags);
    template <typename T>
    FORCE_INLINE T *createComponent(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags flags);

private:
    Object *componentFromClass(CBEClass clazz, const TChar *componentName, EObjectFlags flags);
    Object *componentFromTemplate(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags flags);
} META_ANNOTATE(NoExport);

template <typename T>
FORCE_INLINE T *Actor::createComponent(const TChar *componentName, EObjectFlags flags)
{
    Object *component = componentFromClass(T::staticType(), componentName, flags);
    T *comp = cast<T>(component);
    if (comp)
    {
        addComponent(comp);
    }
    else
    {
        component->beginDestroy();
    }
    return comp;
}
template <typename T>
FORCE_INLINE T *Actor::createComponent(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags flags)
{
    Object *component = componentFromTemplate(objTemplate, componentName, flags);
    T *comp = cast<T>(component);
    if (comp)
    {
        addComponent(comp);
    }
    else
    {
        component->beginDestroy();
    }
    return comp;
}

} // namespace cbe