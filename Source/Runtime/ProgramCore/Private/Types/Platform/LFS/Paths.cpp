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

const TChar *Paths::applicationName()
{
    static const String appName = PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(FileSystemFunctions::applicationPath()));
    return appName.getChar();
}

const TChar *Paths::engineRoot()
{
    static const String rootPath = PathFunctions::toAbsolutePath(TCHAR(".."), Paths::applicationDirectory());
    return rootPath.getChar();
}

const TChar *Paths::engineRuntimeRoot()
{
    static const String runtimeRootPath = PathFunctions::combinePath(engineRoot(), TCHAR("Runtime"));
    return runtimeRootPath.getChar();
}
