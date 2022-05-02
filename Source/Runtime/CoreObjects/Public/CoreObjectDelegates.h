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
namespace CBE
{
class Object;
}

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

    using PackageEvent = Event<CoreObjectDelegates, CBE::Object *>;
    static PackageEvent onPackageSaved;
    static PackageEvent onPackageLoaded;
    FORCE_INLINE static void broadcastPackageSaved(CBE::Object *package) { onPackageSaved.invoke(package); }
    FORCE_INLINE static void broadcastPackageLoaded(CBE::Object *package) { onPackageLoaded.invoke(package); }
};
