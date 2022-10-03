/*!
 * \file EditorHelpers.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEEditorExports.h"
#include "CBEObjectTypes.h"
#include "String/String.h"

class FieldProperty;

namespace cbe
{
class Object;
class ObjectTemplate;
class ActorPrefab;
class StaticMesh;
class World;
class Actor;
class TransformComponent;
} // namespace cbe

class CBEEDITOR_EXPORT EditorHelpers
{
private:
    EditorHelpers() = default;

public:
    // Returns the root actor to which all this static mesh actors are attached to
    static cbe::Actor *
        addStaticMeshesToWorld(const std::vector<cbe::StaticMesh *> &staticMeshes, cbe::World *world, const String &rootActorName);

    static cbe::Actor *addActorToWorld(cbe::World *world, CBEClass actorClass, const String &actorName, EObjectFlags flags);
    static cbe::Actor *addActorToWorld(cbe::World *world, cbe::ActorPrefab *inPrefab, const String &name, EObjectFlags flags);

    static cbe::Object *addComponentToPrefab(cbe::ActorPrefab *prefab, CBEClass compClass, const String &compName);
    static cbe::Object *addComponentToPrefab(cbe::ActorPrefab *prefab, cbe::ObjectTemplate *compTemplate, const String &compName);
    static void removeComponentFromPrefab(cbe::ActorPrefab *prefab, cbe::Object *comp);
    static cbe::Object *modifyComponentInPrefab(cbe::ActorPrefab *prefab, cbe::Object *modifyingComp);

    // Just few helpers that composes and adds to modifyComponent(), Must be called before modifying for component. Actors always has overrides
    static cbe::Object *modifyPrefabCompField(const FieldProperty *prop, cbe::Object *comp);
    static cbe::Object *resetPrefabCompField(const FieldProperty *prop, cbe::Object *comp);

private:
    static void postAddActorToWorld(cbe::World *world, cbe::ActorPrefab *prefab);
    static void removeActorFromWorld(cbe::World *world, cbe::Actor *actor);

    static void componentAddedToWorld(cbe::World *world, cbe::Actor *actor, cbe::Object *component);
    static void componentRemovedFromWorld(cbe::World *world, cbe::Actor *actor, cbe::Object *component);

    static void attachActorInWorld(cbe::World *world, cbe::Actor *attachingActor, cbe::TransformComponent *attachToComp);
    static void detachActorInWorld(cbe::World *world, cbe::Actor *detachingActor);
};