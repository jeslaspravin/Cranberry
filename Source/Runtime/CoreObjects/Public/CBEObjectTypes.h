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
#include "Types/CoreTypes.h"

#include "CoreObjectsExports.h"

class ClassProperty;

using ObjectAllocIdx = uint32;
using EObjectFlags = uint64;
using CBEClass = const ClassProperty *;

namespace CBE
{
class Object;

enum EObjectFlagBits : EObjectFlags
{
    /*
     * Default object that are created as part of
     * CBE::ObjectAllocatorBase creation and are not cleared during garbage collection
     */
    Default = 0x00'00'00'00'00'00'00'01,
    /*
     * Object when marked for delete will be deleted during later
     * garbage collection no matter if they are referred or not
     */
    MarkedForDelete = 0x00'00'00'00'00'00'00'02,
    /*
     * Object after deleted will be marked as deleted, deleted object
     * remains available until the allocated slot is entirely deleted
     */
    Deleted = 0x00'00'00'00'00'00'00'04,
    /*
     * Objects marked as root can only be removed if its parent is deleted or if it is manually deleted.
     * GC skips over objects marked as root
     */
    RootObject = 0x00'00'00'00'00'00'00'08,
    /*
     * If package is modified and needs to be saved
     */
    PackageDirty = 0x00'00'00'00'00'00'00'10,
    /*
     * If object of the package is being loaded/needs loading. Once object is loaded this flag will be cleared
     */
    PackageLoadPending = 0x00'00'00'00'00'00'00'20
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

} // namespace CBE