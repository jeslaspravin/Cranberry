/*!
 * \file WACHelpers.h
 *
 * \author Jeslas
 * \date May 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "Math/CoreMathTypes.h"
#include "Math/Transform3D.h"

#include <vector>

namespace cbe
{
class Actor;
class TransformComponent;
class TransformLeafComponent;
class World;

/**
 * World Actor Components helpers
 */
class ENGINECORE_EXPORT WACHelpers
{
private:
    WACHelpers() = default;

public:
    //////////////////////////////////////////////////////////////////////////
    // World helpers
    //////////////////////////////////////////////////////////////////////////
    static TransformComponent *getActorAttachedToComp(const World *thisWorld, const Actor *actor);
    static Actor *getActorAttachedTo(const World *thisWorld, const Actor *actor);

    //////////////////////////////////////////////////////////////////////////
    // Actor helpers
    //////////////////////////////////////////////////////////////////////////
    static void attachActor(Actor *thisActor, TransformComponent *attachToComp);
    static Actor *getActorAttachedTo(const Actor *thisActor);
    static void detachActor(Actor *thisActor);

    //////////////////////////////////////////////////////////////////////////
    // TransformComponent helpers
    //////////////////////////////////////////////////////////////////////////
    static void attachComponent(TransformComponent *thisComp, TransformComponent *attachToComp);
    static TransformComponent *getComponentAttachedTo(const TransformComponent *thisComp);
    static void getComponentChildren(
        const TransformComponent *thisComp, std::vector<TransformComponent *> &tfComps, std::vector<TransformLeafComponent *> &leafComps
    );
    static void getComponentTransformChilds(const TransformComponent *thisComp, std::vector<TransformComponent *> &tfComps);
    static void getComponentLeafs(const TransformComponent *thisComp, std::vector<TransformLeafComponent *> &leafComps);
    static void detachComponent(TransformComponent *thisComp);

    static void componentTransformed(TransformComponent *thisComp);
    static void setComponentWorldLocation(TransformComponent *thisComp, Vector3 location);
    static void setComponentWorldRotation(TransformComponent *thisComp, Rotation rotation);
    static void setComponentWorldScale(TransformComponent *thisComp, Vector3 scale);
    static void setComponentWorldTransform(TransformComponent *thisComp, Transform3D newTf);
    static Vector3 getComponentWorldLocation(const TransformComponent *thisComp);
    static Rotation getComponentWorldRotation(const TransformComponent *thisComp) { return getComponentWorldRotationQ(thisComp).toRotation(); }
    static Quat getComponentWorldRotationQ(const TransformComponent *thisComp);
    static Vector3 getComponentWorldScale(const TransformComponent *thisComp);
    static Transform3D getComponentWorldTransform(const TransformComponent *thisComp);

    //////////////////////////////////////////////////////////////////////////
    // TransformLeafComponent helpers
    //////////////////////////////////////////////////////////////////////////
    static void attachComponent(TransformLeafComponent *thisComp, TransformComponent *attachToComp);
    static void detachComponent(TransformLeafComponent *thisComp);
};

} // namespace cbe