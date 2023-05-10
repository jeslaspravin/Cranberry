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
#include "WACHelpers.h"

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
class ENGINECORE_EXPORT TransformComponent final : public Object
{
    GENERATED_CODES()

private:
    META_ANNOTATE()
    Transform3D relativeTf;

public:
    void setRelativeLocation(Vector3 location)
    {
        relativeTf.setTranslation(location);
        WACHelpers::componentTransformed(this);
    }
    void setRelativeRotation(Rotation rotation)
    {
        relativeTf.setRotation(rotation);
        WACHelpers::componentTransformed(this);
    }
    void setRelativeScale(Vector3 scale)
    {
        relativeTf.setScale(scale);
        WACHelpers::componentTransformed(this);
    }
    void setRelativeTransform(Transform3D newRelativeTf)
    {
        relativeTf = newRelativeTf;
        WACHelpers::componentTransformed(this);
    }
    void setWorldLocation(Vector3 location) { WACHelpers::setComponentWorldLocation(this, location); }
    void setWorldRotation(Rotation rotation) { WACHelpers::setComponentWorldRotation(this, rotation); }
    void setWorldScale(Vector3 scale) { WACHelpers::setComponentWorldScale(this, scale); }
    void setWorldTransform(Transform3D newTf) { WACHelpers::setComponentWorldTransform(this, newTf); }

    FORCE_INLINE const Transform3D &getRelativeTransform() const { return relativeTf; }
    Vector3 getWorldLocation() { return WACHelpers::getComponentWorldLocation(this); }
    Rotation getWorldRotation() { return WACHelpers::getComponentWorldRotation(this); }
    Vector3 getWorldScale() { return WACHelpers::getComponentWorldScale(this); }
    Transform3D getWorldTransform() const { return WACHelpers::getComponentWorldTransform(this); }

    TransformComponent *getAttachedTo() const { return WACHelpers::getComponentAttachedTo(this); }

    World *getWorld() const;
    Actor *getActor() const;

} META_ANNOTATE(NoExport);

class ENGINECORE_EXPORT TransformLeafComponent : public Object
{
    GENERATED_CODES()
private:
    META_ANNOTATE()
    TransformComponent *attachedTo = nullptr;

public:
    void setAttachedTo(TransformComponent *otherComp) { attachedTo = otherComp; }
    TransformComponent *getAttachedTo() const { return attachedTo; }

    FORCE_INLINE const Transform3D &getRelativeTransform() const { return attachedTo->getRelativeTransform(); }
    Vector3 getWorldLocation() { return attachedTo->getWorldLocation(); }
    Rotation getWorldRotation() { return attachedTo->getWorldRotation(); }
    Vector3 getWorldScale() { return attachedTo->getWorldScale(); }
    FORCE_INLINE Transform3D getWorldTransform() const { return attachedTo->getWorldTransform(); }

    World *getWorld() const;
    Actor *getActor() const;
} META_ANNOTATE(NoExport);

} // namespace cbe
