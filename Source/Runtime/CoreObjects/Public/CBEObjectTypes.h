/*!
 * \file CBEObjectTypes.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/StringID.h"
#include "Types/HashTypes.h"
#include "Types/CoreTypes.h"

#include "CoreObjectsExports.h"

class ClassProperty;

using ObjectAllocIdx = uint32;
using EObjectFlags = uint64;
using CBEClass = const ClassProperty *;

namespace cbe
{
class Object;

enum EObjectFlagBits : EObjectFlags
{
    /**
     * Default object that are created as part of
     * cbe::ObjectAllocatorBase creation and are not cleared during garbage collection
     */
    ObjFlag_Default = 0x00'00'00'00'00'00'00'01,
    /**
     * Object when marked for delete will be deleted during later
     * garbage collection no matter if they are referred or not
     */
    ObjFlag_MarkedForDelete = 0x00'00'00'00'00'00'00'02,
    /**
     * Object after deleted will be marked as deleted, deleted object
     * remains available until the allocated slot is entirely deleted.
     * However It is not safe to use object in this state.
     */
    ObjFlag_Deleted = 0x00'00'00'00'00'00'00'04,
    /**
     * Object will be marked as GCPurge if destroy is part of whole clean up and other systems might not provide necessary services at this
     * point
     */
    ObjFlag_GCPurge = 0x00'00'00'00'00'00'00'08,
    /**
     * Objects marked as root can only be removed if its parent is deleted or if it is manually deleted.
     * GC skips over objects marked as root
     */
    ObjFlag_RootObject = 0x00'00'00'00'00'00'00'10,
    /**
     * If Object is transient and will not be serialized with package
     */
    ObjFlag_Transient = 0x00'00'00'00'00'00'00'20,
    /**
     * If package is modified and needs to be saved
     */
    ObjFlag_PackageDirty = 0x00'00'00'00'00'00'00'40,
    /**
     * If object of the package is being loaded/needs loading. Once object is loaded this flag will be cleared
     */
    ObjFlag_PackageLoadPending = 0x00'00'00'00'00'00'00'80,
    /**
     * If object of the package is Loaded. Once object is loaded this flag will be set
     */
    ObjFlag_PackageLoaded = 0x00'00'00'00'00'00'01'00,
    /**
     * If object is a template default object
     */
    ObjFlag_TemplateDefault = 0x00'00'00'00'00'00'02'00,
    /**
     * If object is created from Template.
     */
    ObjFlag_FromTemplate = 0x00'00'00'00'00'00'04'00
};

enum class EObjectTraversalMode
{
    OnlyObject,
    // If we want to traverse all the sub objects of Object, but not grand children and after
    ObjectAndChildren,
    EntireObjectTree
};

// Why separate accessor? Because this accessor will be needed only for some low level carefully
// orchestrated code and discourage its use for gameplay
class COREOBJECTS_EXPORT INTERNAL_ObjectCoreAccessors
{
private:
    INTERNAL_ObjectCoreAccessors() = default;

public:
    static EObjectFlags &getFlags(Object *object);
    static ObjectAllocIdx getAllocIdx(const Object *object);
    static void setAllocIdx(Object *object, ObjectAllocIdx allocIdx);
    // clazz is just class property of this object and is used only when creating the object for the
    // first time
    static void setOuterAndName(Object *object, const String &newName, Object *outer, CBEClass clazz = nullptr);
    // Just some additional helper
    static void setOuter(Object *object, Object *outer);
    static void renameObject(Object *object, const String &newName);
};

} // namespace cbe