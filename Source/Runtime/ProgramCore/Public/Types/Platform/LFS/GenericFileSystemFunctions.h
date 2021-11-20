#pragma once
#include "String/String.h"

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

    template <typename... Paths>
    static String combinePath(Paths... paths)
    {
        return FileSystemType::combinePath(paths...);
    }

    static std::vector<String> listAllFiles(const String& directory, bool bRecursive)
    {
        return FileSystemType::listAllFiles(directory, bRecursive);
    }

    static bool moveFile(GenericFile* moveFrom, GenericFile* moveTo) {
        return FileSystemType::moveFile(moveFrom, moveTo);
    }

    static bool copyFile(GenericFile* copyFrom, GenericFile* copyTo) {
        return FileSystemType::moveFile(copyFrom, copyTo);
    }
    static bool replaceFile(GenericFile* replaceWith, GenericFile* replacing,GenericFile* backupFile) {
        return FileSystemType::replaceFile(replaceWith,replacing,backupFile);
    }

    static String stripExtension(const String& fileName, String& extension) {
        String::size_type foundAt = fileName.rfind('.', fileName.length());

        if (foundAt != String::npos) {
            String newFileName = fileName.substr(0, foundAt);
            extension = fileName.substr(foundAt + 1);
            return newFileName;
        }
        return fileName;
    }
};