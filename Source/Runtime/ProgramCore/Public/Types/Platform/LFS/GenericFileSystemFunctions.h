/*!
 * \file GenericFileSystemFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

class GenericFile;
class String;

template <class _Ty, class _Alloc>
class std::vector;

template <typename FileSystemType>
class GenericFileSystemFunctions
{
private:
    GenericFileSystemFunctions() = default;

public:
    static String applicationPath() { return FileSystemType::applicationPath(); }

    NODISCARD static std::vector<String> listAllFiles(const String &directory, bool bRecursive)
    {
        return FileSystemType::listAllFiles(directory, bRecursive);
    }

    NODISCARD static std::vector<String> listFiles(const String &directory, bool bRecursive, const String &wildcard = TCHAR("*"))
    {
        return FileSystemType::listFiles(directory, bRecursive, wildcard);
    }

    NODISCARD static std::vector<String> listAllDirectories(const String &directory, bool bRecursive)
    {
        return FileSystemType::listAllDirectories(directory, bRecursive);
    }

    static bool moveFile(GenericFile *moveFrom, GenericFile *moveTo) { return FileSystemType::moveFile(moveFrom, moveTo); }

    static bool copyFile(GenericFile *copyFrom, GenericFile *copyTo) { return FileSystemType::moveFile(copyFrom, copyTo); }
    static bool replaceFile(GenericFile *replaceWith, GenericFile *replacing, GenericFile *backupFile)
    {
        return FileSystemType::replaceFile(replaceWith, replacing, backupFile);
    }

    NODISCARD static bool exists(const TChar *fullPath) { return FileSystemType::exists(fullPath); }
    NODISCARD static bool fileExists(const TChar *fullPath) { return FileSystemType::fileExists(fullPath); }
    NODISCARD static bool dirExists(const TChar *fullPath) { return FileSystemType::dirExists(fullPath); }
};