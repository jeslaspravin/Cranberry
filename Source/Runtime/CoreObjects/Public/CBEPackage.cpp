/*!
 * \file CBEPackage.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CBEPackage.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "ObjectPathHelpers.h"

namespace cbe
{

Package::Package()
{
    ObjectPrivateDataView objectDatV = getObjectData();
    debugAssert(!TCharStr::empty(objectDatV.name) || BIT_SET(objectDatV.flags, EObjectFlagBits::ObjFlag_Default));

    // Name will be empty only in default objects now
    if (!TCharStr::empty(objectDatV.name))
    {
        StringView outPackageName;
        packagePath = ObjectPathHelper::splitPackageNameAndPath(outPackageName, objectDatV.name);
        packageName = outPackageName;
    }
}

void Package::setPackageRoot(const String &root) { packageRoot = root; }

String Package::getPackageFilePath() const
{
    ObjectPrivateDataView objectDatV = getObjectData();
    return PathFunctions::combinePath(getPackageRoot(), String(objectDatV.name) + TCHAR(".") + PACKAGE_EXT);
}

void Package::destroy() { Object::destroy(); }

} // namespace cbe