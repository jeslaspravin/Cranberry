/*!
 * \file WACHelpers.cpp
 *
 * \author Jeslas
 * \date May 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WACHelpers.h"
#include "Classes/Actor.h"
#include "Classes/World.h"
#include "Classes/ActorPrefab.h"
#include "Components/ComponentBase.h"

namespace cbe
{
void WACHelpers::attachActor(Actor *thisActor, TransformComponent *attachToComp)
{
    World *world = thisActor->getWorld();
    debugAssertf(
        attachToComp->getWorld() && world, "Attach actor can be done only in world actors[{} <- {}]", thisActor->getObjectData().path,
        attachToComp->getObjectData().path
    );
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        debugAssert(thisActor->getRootComponent());
        attachComponent(thisActor->getRootComponent(), attachToComp);
    }
    else
    {
        attachComponent(ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(thisActor))->getRootComponent(), attachToComp);
    }
}

TransformComponent *WACHelpers::getActorAttachedToComp(const World *thisWorld, const Actor *actor)
{
    debugAssert(thisWorld && actor);

    if (EWorldState::isPreparedState(thisWorld->getState()))
    {
        return thisWorld->getActorAttachedToComp(actor);
    }
    else
    {
        auto itr = thisWorld->actorAttachedTo.find(actor);
        if (itr != thisWorld->actorAttachedTo.cend())
        {
            return itr->second.component;
        }
    }
    return nullptr;
}

Actor *WACHelpers::getActorAttachedTo(const World *thisWorld, const Actor *actor)
{
    debugAssert(thisWorld && actor);

    if (EWorldState::isPreparedState(thisWorld->getState()))
    {
        return thisWorld->getActorAttachedTo(actor);
    }
    else
    {
        auto itr = thisWorld->actorAttachedTo.find(actor);
        if (itr != thisWorld->actorAttachedTo.cend())
        {
            return itr->second.actor;
        }
    }
    return nullptr;
}

Actor *WACHelpers::getActorAttachedTo(const Actor *thisActor)
{
    if (World *world = thisActor->getWorld())
    {
        return getActorAttachedTo(world, thisActor);
    }

    // If not in world there is no way this actor could be attached to another actor's component
    return nullptr;
}

void WACHelpers::detachActor(Actor *thisActor)
{
    debugAssert(thisActor->getRootComponent());
    detachComponent(thisActor->getRootComponent());
}

void WACHelpers::attachComponent(TransformComponent *thisComp, TransformComponent *attachToComp)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        /**
         * World is prepared for play we just care about attachment information present inside the World.
         * So directly modifying transform attachment is enough
         */
        world->tfAttachmentChanged(thisComp, attachToComp);
    }
    else
    {
        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        ActorPrefab *attach2Prefab = ActorPrefab::prefabFromComponent(attachToComp);
        debugAssertf(thisPrefab && attach2Prefab, "Prefabs must be valid, If the actor is properly created!");

        if (thisPrefab == attach2Prefab)
        {
            debugAssert(thisComp != thisPrefab->getRootComponent());
            thisPrefab->setComponentAttachedTo(thisComp, attachToComp);
        }
        else
        {
            // If not same prefab then we must be trying to attach actor's root to another actor's component
            debugAssertf(
                world && thisPrefab->getRootComponent() == thisComp,
                "World must be valid when attaching actors together and attaching component must be root"
            );

            world->actorAttachedTo[thisPrefab->getActorTemplate()] = { attach2Prefab->getActorTemplate(), attachToComp };
        }
    }
}

TransformComponent *WACHelpers::getComponentAttachedTo(const TransformComponent *thisComp)
{
    ActorPrefab *thisActorPrefab = ActorPrefab::prefabFromComponent(thisComp);
    Actor *thisActor = thisActorPrefab->getActorTemplate();
    debugAssert(thisActor);
    World *world = thisActor->getWorld();

    if (thisComp == thisActorPrefab->getRootComponent())
    {
        return world ? getActorAttachedToComp(world, thisActor) : nullptr;
    }
    else
    {
        if (world && EWorldState::isPreparedState(world->getState()))
        {
            return world->getComponentAttachedTo(thisComp);
        }
        else
        {
            return thisActorPrefab->getAttachedToComp(thisComp);
        }
    }
}

void WACHelpers::getComponentChildren(
    const TransformComponent *thisComp, std::vector<TransformComponent *> &tfComps, std::vector<TransformLeafComponent *> &leafComps
)
{
    Actor *thisActor = thisComp->getActor();

    // First add all the native added components.
    // This is necessary even if prepared case as in that state this will be enough for child leaf components
    leafComps.reserve(leafComps.size() + thisActor->getLeafComponents().size());
    for (TransformLeafComponent *leaf : thisActor->getLeafComponents())
    {
        if (leaf->getAttachedTo() == thisComp)
        {
            leafComps.push_back(leaf);
        }
    }

    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        world->getComponentAttaches(thisComp, tfComps);
    }
    else
    {
        if (world)
        {
            // Get root component from other attached actors
            for (const std::pair<Actor *const, World::ActorAttachedToInfo> &attachmentPair : world->actorAttachedTo)
            {
                if (attachmentPair.second.component == thisComp)
                {
                    // Have to get from prefab to consider the overridden root component
                    tfComps.push_back(
                        ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(attachmentPair.first))->getRootComponent()
                    );
                }
            }
        }

        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        thisPrefab->getCompAttaches(thisComp, tfComps);
        thisPrefab->getCompAttaches(thisComp, leafComps);
    }
}

void WACHelpers::getComponentTransformChilds(const TransformComponent *thisComp, std::vector<TransformComponent *> &tfComps)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        world->getComponentAttaches(thisComp, tfComps);
    }
    else
    {
        if (world)
        {
            // Get root component from other attached actors
            for (const std::pair<Actor *const, World::ActorAttachedToInfo> &attachmentPair : world->actorAttachedTo)
            {
                if (attachmentPair.second.component == thisComp)
                {
                    // Have to get from prefab to consider the overridden root component
                    tfComps.push_back(
                        ActorPrefab::prefabFromActorTemplate(ActorPrefab::objectTemplateFromObj(attachmentPair.first))->getRootComponent()
                    );
                }
            }
        }

        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        thisPrefab->getCompAttaches(thisComp, tfComps);
    }
}

void WACHelpers::getComponentLeafs(const TransformComponent *thisComp, std::vector<TransformLeafComponent *> &leafComps)
{
    Actor *thisActor = thisComp->getActor();

    // First add all the native added components.
    // This is necessary even if prepared case as in that state this will be enough for child leaf components
    for (TransformLeafComponent *leaf : thisActor->getLeafComponents())
    {
        if (leaf->getAttachedTo() == thisComp)
        {
            leafComps.push_back(leaf);
        }
    }

    World *world = thisComp->getWorld();
    if (!world || !EWorldState::isPreparedState(world->getState()))
    {
        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        thisPrefab->getCompAttaches(thisComp, leafComps);
    }
}

void WACHelpers::detachComponent(TransformComponent *thisComp)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        world->tfAttachmentChanged(thisComp, nullptr);
    }
    else
    {
        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        debugAssertf(thisPrefab, "Prefabs must be valid, If the actor is properly created!");

        if (thisComp == thisPrefab->getRootComponent())
        {
            debugAssert(world);
            // If root component then it must be detached in the world
            world->actorAttachedTo.erase(thisPrefab->getActorTemplate());
        }
        else
        {
            thisPrefab->setComponentAttachedTo(thisComp, nullptr);
        }
    }
}

void WACHelpers::componentTransformed(TransformComponent *thisComp)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        world->tfCompTransformed(thisComp);
    }
}

void WACHelpers::setComponentWorldLocation(TransformComponent *thisComp, Vector3 location)
{
    World *world = thisComp->getWorld();
    Vector3 relativeTranslation = location;
    if (world)
    {
        // If there is a world all component even root has chance to be child of another component
        TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
        if (attachedToComp)
        {
            relativeTranslation = location - getComponentWorldLocation(attachedToComp);
        }
    }
    else
    {
        // Just go down the tree to find the world transform(in prefab space) and set relative value
        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        debugAssertf(thisPrefab, "Prefabs must be valid, If the actor is properly created!");

        Vector3 parentWorldT;
        TransformComponent *parentComp = thisPrefab->getAttachedToComp(thisComp);
        while (parentComp)
        {
            parentWorldT += parentComp->getRelativeTransform().getTranslation();
            parentComp = thisPrefab->getAttachedToComp(parentComp);
        }

        relativeTranslation = location - parentWorldT;
    }

    thisComp->setRelativeLocation(relativeTranslation);
}

void WACHelpers::setComponentWorldRotation(TransformComponent *thisComp, Rotation rotation)
{
    World *world = thisComp->getWorld();
    Quat relativeQ = Quat::fromRotation(rotation);
    debugAssert(!relativeQ.isNan());
    if (world)
    {
        // If there is a world all component even root has chance to be child of another component
        TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
        if (attachedToComp)
        {
            relativeQ = getComponentWorldRotationQ(attachedToComp).inverse() * relativeQ;
        }
    }
    else
    {
        // Just go down the tree to find the world transform(in prefab space) and set relative value
        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        debugAssertf(thisPrefab, "Prefabs must be valid, If the actor is properly created!");

        Quat parentWorldQ;
        TransformComponent *parentComp = thisPrefab->getAttachedToComp(thisComp);
        while (parentComp)
        {
            parentWorldQ = Quat::fromRotation(parentComp->getRelativeTransform().getRotation()) * parentWorldQ;
            parentComp = thisPrefab->getAttachedToComp(parentComp);
        }

        relativeQ = parentWorldQ.inverse() * relativeQ;
    }

    thisComp->setRelativeRotation(relativeQ.toRotation());
}

void WACHelpers::setComponentWorldScale(TransformComponent *thisComp, Vector3 scale)
{
    World *world = thisComp->getWorld();
    Vector3 relativeScale = scale;
    if (world)
    {
        // If there is a world all component even root has chance to be child of another component
        TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
        if (attachedToComp)
        {
            relativeScale = scale * getComponentWorldScale(attachedToComp).safeInverse();
        }
    }
    else
    {
        // Just go down the tree to find the world transform(in prefab space) and set relative value
        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        debugAssertf(thisPrefab, "Prefabs must be valid, If the actor is properly created!");

        Vector3 parentWorldS;
        TransformComponent *parentComp = thisPrefab->getAttachedToComp(thisComp);
        while (parentComp)
        {
            parentWorldS *= parentComp->getRelativeTransform().getScale().safeInverse();
            parentComp = thisPrefab->getAttachedToComp(parentComp);
        }

        relativeScale = scale * parentWorldS;
    }

    thisComp->setRelativeScale(relativeScale);
}

void WACHelpers::setComponentWorldTransform(TransformComponent *thisComp, Transform3D newTf)
{
    World *world = thisComp->getWorld();
    Transform3D relativeTf = newTf;
    if (world)
    {
        // If there is a world all component even root has chance to be child of another component
        TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
        if (attachedToComp)
        {
            relativeTf = getComponentWorldTransform(attachedToComp).invTransform(newTf);
        }
    }
    else
    {
        // Just go down the tree to find the world transform(in prefab space) and set relative value
        ActorPrefab *thisPrefab = ActorPrefab::prefabFromComponent(thisComp);
        debugAssertf(thisPrefab, "Prefabs must be valid, If the actor is properly created!");

        Transform3D parentWorldTf;
        TransformComponent *parentComp = thisPrefab->getAttachedToComp(thisComp);
        while (parentComp)
        {
            parentWorldTf = parentComp->getRelativeTransform().transform(parentWorldTf);
            parentComp = thisPrefab->getAttachedToComp(parentComp);
        }

        relativeTf = parentWorldTf.invTransform(newTf);
    }

    thisComp->setRelativeTransform(relativeTf);
}

Vector3 WACHelpers::getComponentWorldLocation(const TransformComponent *thisComp)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        debugAssert(world->hasWorldTf(thisComp));
        return world->getWorldTf(thisComp).getTranslation();
    }
    Vector3 relativeTl = thisComp->getRelativeTransform().getTranslation();
    TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
    // Does the transform peeling from inside out
    while (attachedToComp)
    {
        relativeTl += attachedToComp->getRelativeTransform().getTranslation();
        attachedToComp = getComponentAttachedTo(attachedToComp);
    }
    return relativeTl;
}

Quat WACHelpers::getComponentWorldRotationQ(const TransformComponent *thisComp)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        debugAssert(world->hasWorldTf(thisComp));
        return world->getWorldTf(thisComp).getRotation();
    }
    Quat relativeRot = Quat::fromRotation(thisComp->getRelativeTransform().getRotation());
    TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
    // Does the transform peeling from inside out
    while (attachedToComp)
    {
        relativeRot = Quat::fromRotation(attachedToComp->getRelativeTransform().getRotation()) * relativeRot;
        attachedToComp = getComponentAttachedTo(attachedToComp);
    }
    return relativeRot;
}

Vector3 WACHelpers::getComponentWorldScale(const TransformComponent *thisComp)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        debugAssert(world->hasWorldTf(thisComp));
        return world->getWorldTf(thisComp).getScale();
    }
    Vector3 relativeScale = thisComp->getRelativeTransform().getScale();
    TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
    while (attachedToComp)
    {
        relativeScale *= attachedToComp->getRelativeTransform().getScale();
        attachedToComp = getComponentAttachedTo(attachedToComp);
    }
    return relativeScale;
}

Transform3D WACHelpers::getComponentWorldTransform(const TransformComponent *thisComp)
{
    World *world = thisComp->getWorld();
    if (world && EWorldState::isPreparedState(world->getState()))
    {
        debugAssert(world->hasWorldTf(thisComp));
        return world->getWorldTf(thisComp);
    }
    // At the end will hold world relative transform which will be the world transform
    Transform3D relativeTf = thisComp->getRelativeTransform();
    TransformComponent *attachedToComp = getComponentAttachedTo(thisComp);
    // Does the transform peeling from inside out
    while (attachedToComp)
    {
        relativeTf = attachedToComp->getRelativeTransform().transform(relativeTf);
        attachedToComp = getComponentAttachedTo(attachedToComp);
    }
    return relativeTf;
}

void WACHelpers::attachComponent(TransformLeafComponent *thisComp, TransformComponent *attachToComp) { thisComp->setAttachedTo(attachToComp); }

void WACHelpers::detachComponent(TransformLeafComponent *thisComp) { thisComp->setAttachedTo(nullptr); }
} // namespace cbe
