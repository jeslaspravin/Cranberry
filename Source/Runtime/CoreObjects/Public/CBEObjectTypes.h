/*!
 * \file CBEObjectTypes.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
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
using ObjectDbIdx = uint64;
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
    ObjFlag_MarkedForDelete = ObjFlag_Default << 1,
    /**
     * Object will be marked as GCPurge if destroy is part of whole clean up and other systems might not provide necessary services at this
     * point
     */
    ObjFlag_GCPurge = ObjFlag_MarkedForDelete << 1,
    /**
     * Objects marked as root can only be removed if its parent is deleted or if it is manually deleted.
     * GC skips over objects marked as root
     */
    ObjFlag_RootObject = ObjFlag_GCPurge << 1,
    /**
     * If Object is transient and will not be serialized with package
     */
    ObjFlag_Transient = ObjFlag_RootObject << 1,
    /**
     * If package is modified and needs to be saved
     */
    ObjFlag_PackageDirty = ObjFlag_Transient << 1,
    /**
     * If object of the package is being loaded/needs loading. Once object is loaded this flag will be cleared
     */
    ObjFlag_PackageLoadPending = ObjFlag_PackageDirty << 1,
    /**
     * If object of the package is Loaded. Once object is loaded this flag will be set
     */
    ObjFlag_PackageLoaded = ObjFlag_PackageLoadPending << 1,
    /**
     * If object is a template default object
     */
    ObjFlag_TemplateDefault = ObjFlag_PackageLoaded << 1,
    /**
     * If object is created from Template.
     */
    ObjFlag_FromTemplate = ObjFlag_TemplateDefault << 1
};

enum class EObjectTraversalMode
{
    // This option can also be used for shallow copy per object
    OnlyObject,
    // If we want to traverse all the sub objects of Object, but not grand children and after
    ObjectAndChildren,
    EntireObjectTree
};

// Never hold copy or reference of this struct unless you are sure all StringView will be alive
struct ObjectPrivateDataView
{
    // Be aware when persisting this view, as there is chance below string view gets invalidated with CoreObjectsDB changes
    // Not using string view as it is not easier to log with printf
    const TChar *name;
    const TChar *path;
    EObjectFlags flags;
    ObjectDbIdx outerIdx;
    StringID sid;
    ObjectAllocIdx allocIdx;
    CBEClass clazz = nullptr;

    static ObjectPrivateDataView getInvalid() { return { .sid = StringID::INVALID, .clazz = nullptr }; }
    constexpr bool isValid() const { return !(sid == StringID::INVALID || clazz == nullptr); }
    constexpr operator bool () const { return isValid(); }
};

// Why separate accessor? Because this accessor will be needed only for some low level carefully
// orchestrated code and discourage its use for gameplay
class COREOBJECTS_EXPORT INTERNAL_ObjectCoreAccessors
{
private:
    INTERNAL_ObjectCoreAccessors() = default;

public:
    // Be aware when persisting this reference, as there is chance below string view gets invalidated with CoreObjectsDB changes
    static EObjectFlags &getFlags(Object *object);

    static ObjectAllocIdx getAllocIdx(const Object *object);
    static void setAllocIdx(Object *object, ObjectAllocIdx allocIdx);
    static void setDbIdx(Object *object, ObjectDbIdx dbIdx);

    // clazz is just class property of this object and is used only when creating the object for the
    // first time
    static void setOuterAndName(Object *object, StringView newName, Object *outer, CBEClass clazz = nullptr);

    // Just some additional helper
    static void setOuter(Object *object, Object *outer);
    static void renameObject(Object *object, StringView newName);
};

} // namespace cbe