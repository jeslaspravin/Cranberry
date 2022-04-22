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

class COREOBJECTS_EXPORT CoreObjectDelegates
{
private:
    CoreObjectDelegates() = default;

public:
    using ContentDirectoryDelegate = Event<CoreObjectDelegates, const String &>;
    static ContentDirectoryDelegate onContentDirectoryAdded;
    static ContentDirectoryDelegate onContentDirectoryRemoved;

    FORCE_INLINE static void broadcastContentDirectoryAdded(const String &contentDir) { onContentDirectoryAdded.invoke(contentDir); }
    FORCE_INLINE static void broadcastContentDirectoryRemoved(const String &contentDir) { onContentDirectoryRemoved.invoke(contentDir); }
};
