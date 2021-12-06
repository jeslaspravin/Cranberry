#pragma once

#include "Types/Platform/LFS/GenericFileSystemFunctions.h"

class PROGRAMCORE_EXPORT WindowsFileSystemFunctions : public GenericFileSystemFunctions<WindowsFileSystemFunctions>
{
public:
    template <typename FirstType, typename LastType, typename std::enable_if<IsStringTypes<FirstType, LastType>::value, int>::type = 0>
    static String combinePath(FirstType&& firstPath, LastType&& lastPath)
    {
        String returnPath(std::forward<FirstType>(firstPath));
        returnPath.append("\\");
        returnPath.append(std::forward<LastType>(lastPath));
        return returnPath;
    }

    template <typename BaseType,typename... Paths,typename std::enable_if<IsString<BaseType>::value && IsStringTypes<Paths...>::value,int>::type = 0>
    static String combinePath(BaseType&& basePath, Paths... paths)
    {
        String returnPath(std::forward<BaseType>(basePath));
        returnPath.append("\\");
        returnPath.append(combinePath(std::forward<Paths>(paths)...));
        return returnPath;
    }

    static std::vector<String> listAllFiles(const String& directory, bool bRecursive);
    static String applicationDirectory(String &appName);
    static bool moveFile(GenericFile* moveFrom, GenericFile* moveTo);
    static bool copyFile(GenericFile* copyFrom, GenericFile* copyTo);
    static bool replaceFile(GenericFile* replaceWith, GenericFile* replacing, GenericFile* backupFile);

};

namespace LFS 
{
    typedef GenericFileSystemFunctions<WindowsFileSystemFunctions> FileSystemFunctions;
}