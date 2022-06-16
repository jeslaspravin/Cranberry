/*!
 * \file Paths.cpp
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "String/String.h"

FORCE_INLINE String Paths::applicationDirectory()
{
    String appName;
    return PathFunctions::splitFileAndDirectory(appName, FileSystemFunctions::applicationPath());
}

String Paths::applicationDirectory(String &appName, String *extension /*= nullptr*/)
{
    String appDir = PathFunctions::splitFileAndDirectory(appName, FileSystemFunctions::applicationPath());
    if (extension)
    {
        appName = PathFunctions::stripExtension(*extension, appName);
    }
    else
    {
        appName = PathFunctions::stripExtension(appName);
    }
    return appDir;
}

String Paths::applicationName()
{
    return PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(FileSystemFunctions::applicationPath()));
}

String Paths::engineRoot() { return PathFunctions::toAbsolutePath(TCHAR(".."), Paths::applicationDirectory()); }
