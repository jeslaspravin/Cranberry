/*!
 * \file ComponentBase.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Components/ComponentBase.h"
#include "CBEObjectHelpers.h"
#include "Classes/Actor.h"
#include "Classes/World.h"

namespace cbe
{

//////////////////////////////////////////////////////////////////////////
/// LogicComponent implementation
//////////////////////////////////////////////////////////////////////////

World *LogicComponent::getWorld() const { return getActor()->getWorld(); }

//////////////////////////////////////////////////////////////////////////
/// TransformComponent implementation
//////////////////////////////////////////////////////////////////////////

void TransformComponent::attachComponent(TransformComponent *attachToComp)
{
    World *world = getWorld();
    debugAssertf(world, "Must be called only one component in world");
    if (attachedTo != attachToComp)
    {
        setAttachedTo(attachToComp);
        world->tfAttachmentChanged(this, attachToComp);
    }
}

void TransformComponent::detachComponent()
{
    World *world = getWorld();
    debugAssertf(world, "Must be called only one component in world");
    if (attachedTo)
    {
        setAttachedTo(nullptr);
        world->tfAttachmentChanged(this, nullptr);
    }
}

void TransformComponent::invalidateComponent()
{
    World *world = getWorld();
    debugAssertf(world, "Must be called only one component in world");
    if (!bInvalidated)
    {
        getWorld()->tfCompInvalidated(this);
        bInvalidated = true;
    }
}

const Vector3 &TransformComponent::setRelativeLocation(Vector3 location)
{
    relativeTf.setTranslation(location);
    if (World *world = getWorld())
    {
        componentTransformed(world);
    }
    return relativeTf.getTranslation();
}

const Rotation &TransformComponent::setRelativeRotation(Rotation rotation)
{
    relativeTf.setRotation(rotation);
    if (World *world = getWorld())
    {
        componentTransformed(world);
    }
    return relativeTf.getRotation();
}

const Vector3 &TransformComponent::setRelativeScale(Vector3 scale)
{
    relativeTf.setScale(scale);
    if (World *world = getWorld())
    {
        componentTransformed(world);
    }
    return relativeTf.getScale();
}

const Transform3D &TransformComponent::setRelativeTransform(const Transform3D &newRelativeTf)
{
    relativeTf = newRelativeTf;
    if (World *world = getWorld())
    {
        componentTransformed(world);
    }
    return relativeTf;
}

Vector3 TransformComponent::setWorldLocation(Vector3 location)
{
    World *world = getWorld();
    alertAlwaysf(world, "Setting transform in world space is valid only for components in world!");
    if (world)
    {
        TransformComponent *attachedToComp = canonicalAttachedTo();
        if (attachedToComp)
        {
            relativeTf.setTranslation(location - attachedToComp->getWorldLocation());
        }
        else
        {
            relativeTf.setTranslation(location);
        }
        componentTransformed(world);
        return getWorldLocation();
    }
    else
    {
        relativeTf.setTranslation(location);
        return relativeTf.getTranslation();
    }
}

Rotation TransformComponent::setWorldRotation(Rotation rotation)
{
    World *world = getWorld();
    alertAlwaysf(world, "Setting transform in world space is valid only for components in world!");
    if (world)
    {
        TransformComponent *attachedToComp = canonicalAttachedTo();
        if (attachedToComp)
        {
            relativeTf.setRotation((Quat(attachedToComp->getWorldRotation()).inverse() * Quat(rotation)).toRotation());
        }
        else
        {
            relativeTf.setRotation(rotation);
        }
        componentTransformed(world);
        return getWorldRotation();
    }
    else
    {
        relativeTf.setRotation(rotation);
        return relativeTf.getRotation();
    }
}

Vector3 TransformComponent::setWorldScale(Vector3 scale)
{
    World *world = getWorld();
    alertAlwaysf(world, "Setting transform in world space is valid only for components in world!");
    if (world)
    {
        TransformComponent *attachedToComp = canonicalAttachedTo();
        if (attachedToComp)
        {
            relativeTf.setScale(attachedToComp->getWorldScale().safeInverse() * scale);
        }
        else
        {
            relativeTf.setScale(scale);
        }
        componentTransformed(world);
        return getWorldScale();
    }
    else
    {
        relativeTf.setScale(scale);
        return relativeTf.getScale();
    }
}

Transform3D TransformComponent::setWorldTransform(const Transform3D &newTf)
{
    World *world = getWorld();
    alertAlwaysf(world, "Setting transform in world space is valid only for components in world!");
    if (world)
    {
        TransformComponent *attachedToComp = canonicalAttachedTo();
        if (attachedToComp)
        {
            relativeTf = attachedToComp->getWorldTransform().invTransform(newTf);
        }
        else
        {
            relativeTf = newTf;
        }
        componentTransformed(world);
        return getWorldTransform();
    }
    else
    {
        relativeTf = newTf;
        return relativeTf;
    }
}

World *TransformComponent::getWorld() const
{
    if (Actor *actor = getActor())
    {
        return actor->getWorld();
    }
    return nullptr;
}

FORCE_INLINE void TransformComponent::componentTransformed(World *world)
{
    debugAssert(world);

#if EDITOR_BUILD
    markDirty(this);
#endif
    bool bOldTransformed = bInvalidated || bTransformed;
    bTransformed = true;
    world->tfCompTransformed(this, bOldTransformed);
}

} // namespace cbe
