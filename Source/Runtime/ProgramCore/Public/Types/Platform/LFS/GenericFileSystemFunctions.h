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

    static std::vector<String> listAllFiles(const String& directory, bool bRecursive)
    {
        return FileSystemType::listAllFiles(directory, bRecursive);
    }

    static std::vector<String> listFiles(const String& directory, bool bRecursive, const String& wildcard = "*")
    {
        return FileSystemType::listFiles(directory, bRecursive, wildcard);
    }

    static std::vector<String> listAllDirectories(const String& directory, bool bRecursive)
    {
        return FileSystemType::listAllDirectories(directory, bRecursive);
    }

    static bool moveFile(GenericFile* moveFrom, GenericFile* moveTo) 
    {
        return FileSystemType::moveFile(moveFrom, moveTo);
    }

    static bool copyFile(GenericFile* copyFrom, GenericFile* copyTo) 
    {
        return FileSystemType::moveFile(copyFrom, copyTo);
    }
    static bool replaceFile(GenericFile* replaceWith, GenericFile* replacing,GenericFile* backupFile) 
    {
        return FileSystemType::replaceFile(replaceWith,replacing,backupFile);
    }
};