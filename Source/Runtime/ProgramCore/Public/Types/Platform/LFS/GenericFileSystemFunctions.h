/*!
 * \file GenericFileSystemFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/LFS/PathFunctions.h"

class GenericFile;

template <typename FileSystemType>
class GenericFileSystemFunctions
{
private:
    GenericFileSystemFunctions() = default;

public:
    static String applicationDirectory(String &appName)
    {
        return FileSystemType::applicationDirectory(appName);
    }

    static std::vector<String> listAllFiles(const String &directory, bool bRecursive)
    {
        return FileSystemType::listAllFiles(directory, bRecursive);
    }

    static std::vector<String> listFiles(
        const String &directory, bool bRecursive, const String &wildcard = TCHAR("*"))
    {
        return FileSystemType::listFiles(directory, bRecursive, wildcard);
    }

    static std::vector<String> listAllDirectories(const String &directory, bool bRecursive)
    {
        return FileSystemType::listAllDirectories(directory, bRecursive);
    }

    static bool moveFile(GenericFile *moveFrom, GenericFile *moveTo)
    {
        return FileSystemType::moveFile(moveFrom, moveTo);
    }

    static bool copyFile(GenericFile *copyFrom, GenericFile *copyTo)
    {
        return FileSystemType::moveFile(copyFrom, copyTo);
    }
    static bool replaceFile(GenericFile *replaceWith, GenericFile *replacing, GenericFile *backupFile)
    {
        return FileSystemType::replaceFile(replaceWith, replacing, backupFile);
    }
};