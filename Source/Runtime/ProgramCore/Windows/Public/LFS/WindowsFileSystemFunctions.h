#pragma once

#include "Types/Platform/LFS/GenericFileSystemFunctions.h"

class PROGRAMCORE_EXPORT WindowsFileSystemFunctions : public GenericFileSystemFunctions<WindowsFileSystemFunctions>
{
public:

    static std::vector<String> listAllFiles(const String& directory, bool bRecursive);
    static std::vector<String> listFiles(const String& directory, bool bRecursive, const String& wildcard);
    static std::vector<String> listAllDirectories(const String& directory, bool bRecursive);
    static String applicationDirectory(String &appName);
    static bool moveFile(GenericFile* moveFrom, GenericFile* moveTo);
    static bool copyFile(GenericFile* copyFrom, GenericFile* copyTo);
    static bool replaceFile(GenericFile* replaceWith, GenericFile* replacing, GenericFile* backupFile);

};

namespace LFS 
{
    typedef GenericFileSystemFunctions<WindowsFileSystemFunctions> FileSystemFunctions;
}