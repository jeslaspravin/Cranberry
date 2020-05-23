#pragma once

#include "../../LFS/GenericFileSystemFunctions.h"

class WindowsFileSystemFunctions : public GenericFileSystemFunctions<WindowsFileSystemFunctions>
{
public:
    template <typename FirstType, typename LastType, typename std::enable_if<IsStringTypes<FirstType, LastType>::value, int>::type = 0>
    static String combinePath(FirstType firstPath, LastType lastPath)
    {
        String returnPath = firstPath;
        returnPath.append("\\");
        returnPath.append(lastPath);
        return returnPath;
    }

    template <typename BaseType,typename... Paths,typename std::enable_if<IsString<BaseType>::value && IsStringTypes<Paths...>::value,int>::type = 0>
    static String combinePath(const BaseType& basePath, Paths... paths)
    {
        String returnPath = basePath;
        returnPath.append("\\");
        returnPath.append(combinePath(paths...));
        return returnPath;
    }

    static std::vector<String> listAllFiles(const String& directory, bool bRecursive);
    static String applicationDirectory(String &appName);
    static bool moveFile(GenericFile* moveFrom, GenericFile* moveTo);
    static bool copyFile(GenericFile* copyFrom, GenericFile* copyTo);
    static bool replaceFile(GenericFile* replaceWith, GenericFile* replacing, GenericFile* backupFile);

};

namespace LFS {
    typedef GenericFileSystemFunctions<WindowsFileSystemFunctions> FileSystemFunctions;
}