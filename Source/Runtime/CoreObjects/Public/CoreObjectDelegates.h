/*!
 * \file CoreObjectDelegates.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoreObjectsExports.h"
#include "Types/Delegates/Delegate.h"

class String;
class PackageLoader;
namespace cbe
{
class Object;
}

using ObjectGetterDelegate = SingleCastDelegate<cbe::Object *>;
using ObjectSetterDelegate = Delegate<cbe::Object *>;

class COREOBJECTS_EXPORT CoreObjectDelegates
{
private:
    CoreObjectDelegates() = default;

public:
    using ContentDirectoryEvent = Event<CoreObjectDelegates, const String &>;
    static ContentDirectoryEvent onContentDirectoryAdded;
    static ContentDirectoryEvent onContentDirectoryRemoved;

    FORCE_INLINE static void broadcastContentDirectoryAdded(const String &contentDir) { onContentDirectoryAdded.invoke(contentDir); }
    FORCE_INLINE static void broadcastContentDirectoryRemoved(const String &contentDir) { onContentDirectoryRemoved.invoke(contentDir); }

    using PackageLoaderEvent = Event<CoreObjectDelegates, PackageLoader *>;
    using PackageEvent = Event<CoreObjectDelegates, cbe::Object *>;
    static PackageEvent onPackageSaved;
    static PackageEvent onPackageLoaded;
    static PackageEvent onPackageUnloaded;
    /**
     * Broadcasts when package is scanned from directory and is ready to be loaded.
     * No objects except cbe::Package is created at this point however contained objects table is loaded
     */
    static PackageLoaderEvent onPackageScanned;
    FORCE_INLINE static void broadcastPackageSaved(cbe::Object *package) { onPackageSaved.invoke(package); }
    FORCE_INLINE static void broadcastPackageLoaded(cbe::Object *package) { onPackageLoaded.invoke(package); }
    FORCE_INLINE static void broadcastPackageUnloaded(cbe::Object *package) { onPackageUnloaded.invoke(package); }
    FORCE_INLINE static void broadcastPackageScanned(PackageLoader *packageLoader) { onPackageScanned.invoke(packageLoader); }

    // Object related events
    using ObjectEvent = Event<CoreObjectDelegates, cbe::Object *>;
    static ObjectEvent onObjectCreated;
    static ObjectEvent onObjectDestroyed;
    FORCE_INLINE static void broadcastObjectCreated(cbe::Object *obj) { onObjectCreated.invoke(obj); }
    FORCE_INLINE static void broadcastObjectDestroyed(cbe::Object *obj) { onObjectDestroyed.invoke(obj); }
};
