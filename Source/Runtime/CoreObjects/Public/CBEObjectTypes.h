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
    Default = 0x00'00'00'00'00'00'00'01, // Default object that are created as part of
                                         // CBE::ObjectAllocatorBase creation
    MarkedForDelete
        = 0x00'00'00'00'00'00'00'02,    // Object when marked for delete will be deleted during later
                                        // garbage collection no matter if they are referred or not
    Deleted = 0x00'00'00'00'00'00'00'04 // Object after deleted will be marked as deleted, deleted object
                                        // remains available until the allocated slot is entirely deleted
};

// Why separate accessor? Because this accessor will be needed only for some low level carefully
// orchestrated code and discourage its use for gameplay
class COREOBJECTS_EXPORT PrivateObjectCoreAccessors
{
private:
    PrivateObjectCoreAccessors() = default;

public:
    static EObjectFlags &getFlags(Object *object);
    static ObjectAllocIdx getAllocIdx(const Object *object);
    // clazz is just class property of this object and is used only when creating the object for the
    // first time
    static void setOuterAndName(
        Object *object, const String &newName, Object *outer, CBEClass clazz = nullptr);
    // Just some additional helper
    static void setOuter(Object *object, Object *outer);
    static void renameObject(Object *object, const String &newName);
};

} // namespace CBE