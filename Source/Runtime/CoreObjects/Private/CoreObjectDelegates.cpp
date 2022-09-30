/*!
 * \file CoreObjectDelegates.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CoreObjectDelegates.h"

CoreObjectDelegates::ContentDirectoryEvent CoreObjectDelegates::onContentDirectoryAdded;
CoreObjectDelegates::ContentDirectoryEvent CoreObjectDelegates::onContentDirectoryRemoved;

CoreObjectDelegates::PackageEvent CoreObjectDelegates::onPackageSaved;
CoreObjectDelegates::PackageEvent CoreObjectDelegates::onPackageLoaded;
CoreObjectDelegates::PackageEvent CoreObjectDelegates::onPackageUnloaded;

CoreObjectDelegates::PackageLoaderEvent CoreObjectDelegates::onPackageScanned;

CoreObjectDelegates::ObjectEvent CoreObjectDelegates::onObjectCreated;
CoreObjectDelegates::ObjectEvent CoreObjectDelegates::onObjectDestroyed;
