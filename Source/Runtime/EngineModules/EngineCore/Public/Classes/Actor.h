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

#include "Actor.gen.h"

namespace cbe
{
class World;
class LogicComponent;
class TransformComponent;
class TransformLeafComponent;
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
    std::set<TransformLeafComponent *> leafComps;

    // Root component can only be modified at edit time or native constructor
    META_ANNOTATE()
    TransformComponent *rootComponent = nullptr;

public:
    // For any world spawned actor, its outer will be world itself
    World *getWorld() const;
    // Use this getRootComponent only if you are sure this actor is already a part of the prepared world
    FORCE_INLINE TransformComponent *getRootComponent() const { return rootComponent; }
    FORCE_INLINE const std::set<TransformComponent *> &getTransformComponents() const { return transformComps; }
    FORCE_INLINE const std::set<LogicComponent *> &getLogicComponents() const { return logicComps; }
    FORCE_INLINE const std::set<TransformLeafComponent *> &getLeafComponents() const { return leafComps; }

    void addComponent(TransformComponent *component);
    void addComponent(TransformLeafComponent *component);
    void addComponent(LogicComponent *component);
    void removeComponent(TransformComponent *component);
    void removeComponent(TransformLeafComponent *component);
    void removeComponent(LogicComponent *component);

    // Helpers
    template <typename T>
    FORCE_INLINE T *createComponent(const TChar *componentName, EObjectFlags componentFlags);
    template <typename T>
    FORCE_INLINE T *createComponent(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags componentFlags);

private:
    Object *componentFromClass(CBEClass clazz, const TChar *componentName, EObjectFlags componentFlags);
    Object *componentFromTemplate(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags componentFlags);

} META_ANNOTATE(NoExport);

template <typename T>
FORCE_INLINE T *Actor::createComponent(const TChar *componentName, EObjectFlags componentFlags)
{
    Object *component = componentFromClass(T::staticType(), componentName, componentFlags);
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
FORCE_INLINE T *Actor::createComponent(ObjectTemplate *objTemplate, const TChar *componentName, EObjectFlags componentFlags)
{
    Object *component = componentFromTemplate(objTemplate, componentName, componentFlags);
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