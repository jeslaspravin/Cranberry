/*!
 * \file WindowsFileSystemFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/LFS/GenericFileSystemFunctions.h"

class PROGRAMCORE_EXPORT WindowsFileSystemFunctions : public GenericFileSystemFunctions<WindowsFileSystemFunctions>
{
public:
    static std::vector<String> listAllFiles(const String &directory, bool bRecursive);
    static std::vector<String> listFiles(const String &directory, bool bRecursive, const String &wildcard);
    static std::vector<String> listAllDirectories(const String &directory, bool bRecursive);
    static String applicationPath();
    static bool moveFile(GenericFile *moveFrom, GenericFile *moveTo);
    static bool copyFile(GenericFile *copyFrom, GenericFile *copyTo);
    static bool replaceFile(GenericFile *replaceWith, GenericFile *replacing, GenericFile *backupFile);

    static bool exists(const TChar *fullPath);
    static bool fileExists(const TChar *fullPath);
    static bool dirExists(const TChar *fullPath);
};

namespace LFS
{
typedef GenericFileSystemFunctions<WindowsFileSystemFunctions> FileSystemFunctions;
}