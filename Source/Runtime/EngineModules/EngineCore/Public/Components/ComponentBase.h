/*!
 * \file ComponentBase.h
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

#include "ComponentBase.gen.h"

namespace cbe
{
class Actor;
class World;

/*!
 * \class LogicComponent
 *
 * \brief Logic component is used to add logics to actor that it belongs to. This component do not have any transforms and will not be present
 * in transform hierarchy of actor components
 *
 * \author Jeslas
 * \date July 2022
 */
class ENGINECORE_EXPORT LogicComponent : public Object
{
    GENERATED_CODES()

public:
    World *getWorld() const;
    Actor *getActor() const;

} META_ANNOTATE(NoExport);

/*!
 * \class TransformComponent
 *
 * \brief Transform component will be the base every other component that can be moved in level. It do not have any visuals on its own
 *
 * \author Jeslas
 * \date July 2022
 */
class ENGINECORE_EXPORT TransformComponent : public Object
{
    GENERATED_CODES()

protected:
    META_ANNOTATE()
    Transform3D relativeTf;

private:
    META_ANNOTATE(Transient)
    TransformComponent *attachedTo = nullptr;

    // Below flags are used only to avoid multiple trigger of corresponding events. They must be cleared at end of every frame
    bool bInvalidated = false;
    bool bTransformed = false;

public:
    void attachComponent(TransformComponent *attachToComp);
    void detachComponent();
    void setAttachedTo(TransformComponent *otherComp) { attachedTo = otherComp; }

    void invalidateComponent();
    FORCE_INLINE void clearInvalidatedFlag() { bInvalidated = false; }

    const Vector3D &setRelativeLocation(Vector3D location);
    const Rotation &setRelativeRotation(Rotation rotation);
    const Vector3D &setRelativeScale(Vector3D scale);
    const Transform3D &setRelativeTransform(const Transform3D &newRelativeTf);
    Vector3D setWorldLocation(Vector3D location);
    Rotation setWorldRotation(Rotation rotation);
    Vector3D setWorldScale(Vector3D scale);
    Transform3D setWorldTransform(const Transform3D &newTf);
    FORCE_INLINE void clearTransformedFlag() { bTransformed = false; }

    FORCE_INLINE TransformComponent *getAttachedTo() const { return attachedTo; }
    // Searches prefab or world to determine component's actual attachTo even when world is not being played
    TransformComponent *canonicalAttachedTo();

    FORCE_INLINE const Vector3D &getRelativeLocation() const { return relativeTf.getTranslation(); }
    FORCE_INLINE const Rotation &getRelativeRotation() const { return relativeTf.getRotation(); }
    FORCE_INLINE const Vector3D &getRelativeScale() const { return relativeTf.getScale(); }
    FORCE_INLINE const Transform3D &getRelativeTransform() const { return relativeTf; }
    FORCE_INLINE Vector3D getWorldLocation() const { return getWorldTransform().getTranslation(); }
    FORCE_INLINE Rotation getWorldRotation() const { return getWorldTransform().getRotation(); }
    FORCE_INLINE Vector3D getWorldScale() const { return getWorldTransform().getScale(); }
    Transform3D getWorldTransform() const;

    World *getWorld() const;
    Actor *getActor() const;

private:
    FORCE_INLINE void componentTransformed(World *world);

} META_ANNOTATE(NoExport);

} // namespace cbe
