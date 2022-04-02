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

#include "Types/CoreTypes.h"
#include "String/StringID.h"

#include "CoreObjectsExports.h"

using ObjectAllocIdx = uint32;
using EObjectFlags = uint64;

namespace CBE
{
    class Object;

    enum EObjectFlagBits : EObjectFlags
    {
        Deleted = 0x00'00'00'00'00'00'00'01,
        Default = 0x00'00'00'00'00'00'00'02
    };

    // Why separate accessor? Because this accessor will be needed only for some low level carefully orchestrated code and discourage its use for gameplay
    class COREOBJECTS_EXPORT PrivateObjectCoreAccessors
    {
    private:
        PrivateObjectCoreAccessors() = default;
    public:
        static EObjectFlags& getFlags(Object* object);
        static ObjectAllocIdx getAllocIdx(Object* object);
        static void setOuterAndName(Object* object, const String& newName, Object* outer);
        // Just some additional helper
        static void setOuter(Object* object, Object* outer);
        static void renameObject(Object* object, const String& newName);
    };

}