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

    // combinePath(...)
    // Combines given paths with platform specifier separator
    //
    // && (Not necessary but nice to have this)passes the type as it is from the caller like r-values as well else r-values gets converted to l-values on this call
    template <typename... Paths>
    CONST_EXPR static String combinePath(Paths&&... paths)
    {
        return PathFunctions::combinePath(std::forward<Paths>(paths)...);
    }

    static std::vector<String> listAllFiles(const String& directory, bool bRecursive)
    {
        return FileSystemType::listAllFiles(directory, bRecursive);
    }

    static std::vector<String> listFiles(const String& directory, bool bRecursive, const String& wildcard = "*")
    {
        return FileSystemType::listFiles(directory, bRecursive, wildcard);
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