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

namespace CBE
{

Package::Package()
{
    debugAssert(!getName().empty() || BIT_SET(getFlags(), EObjectFlagBits::Default));

    // Name will be empty only in default objects now
    if (!getName().empty())
    {
        packagePath = ObjectPathHelper::splitPackageNameAndPath(packageName, getName().getChar());
    }
}

String Package::getPackageFilePath() const { return PathFunctions::combinePath(getPackageRoot(), getName() + TCHAR(".") + PACKAGE_EXT); }

void Package::destroy() { Object::destroy(); }

} // namespace CBE
