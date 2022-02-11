/*!
 * \file PathFunctions.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <filesystem>

String PathFunctions::toRelativePath(const String& absPath, const String& relToPath)
{
    std::filesystem::path absolutePath(absPath.getChar());
    std::filesystem::path relativeToPath(relToPath.getChar());
    fatalAssert(relativeToPath.is_absolute(), "%s() : Relative to path %s must be absolute path", __func__, relToPath);
    if (absolutePath.is_relative())
    {
        return absPath;
    }
    // Should we implement a toRelativePath platform independent? std::filesystem::relative does that under the hood anyways
    std::error_code errorCode;
    std::filesystem::path relPath = std::filesystem::relative(absolutePath, relativeToPath, errorCode);
    fatalAssert(errorCode.value() == 0, "%s() : Error %s when making [%s] as relative to %s", UTF8_TO_TCHAR(errorCode.message().c_str()), absPath, relToPath);
    return WCHAR_TO_TCHAR(relPath.c_str());
}

String PathFunctions::toAbsolutePath(const String& relPath, const String& basePath)
{
    // Check if path is relative
    {
        std::filesystem::path relativePath(relPath.getChar());
        if (relativePath.is_absolute())
        {
            return relPath;
        }
    }

    const String absPath(PathFunctions::combinePath(basePath, relPath));
    // Replace all "//" and split each path elements
    const std::vector<String> pathElems{ String::split(absPath.replaceAllCopy(TCHAR("\\"), TCHAR("/")), TCHAR("/")) };
    std::vector<String> sanitizedPathElems;
    sanitizedPathElems.reserve(pathElems.size());

    int32 parentDirNum = 0;
    for (int64 i = pathElems.size() - 1; i >= 0; --i)
    {
        if (TCHAR("..") == pathElems[i])
        {
            ++parentDirNum;
        }
        else
        {
            if (parentDirNum == 0)
            {
                sanitizedPathElems.emplace_back(pathElems[i]);
            }
            else
            {
                --parentDirNum;
            }
        }
    }
    std::reverse(sanitizedPathElems.begin(), sanitizedPathElems.end());

    return String::join(sanitizedPathElems.cbegin(), sanitizedPathElems.cend(), TCHAR("/"));
}


bool PathFunctions::isSubdirectory(const String& checkPath, const String& basePath)
{
    const std::vector<String> checkPathElems{ String::split(asGenericPath(checkPath), TCHAR("/")) };
    const std::vector<String> basePathElems{ String::split(asGenericPath(basePath), TCHAR("/")) };

    // If basePath folder count is larger or equal to checkPath then it means basPath can not fit into checkPath. checkPath can never be subdir of basePath
    if (basePathElems.size() >= checkPathElems.size())
    {
        return false;
    }

    for (uint32 i = 0; i < basePathElems.size(); ++i)
    {
        // If any folder do not match then it cannot be subdirectory
        if (checkPathElems[i] != basePathElems[i])
        {
            return false;
        }
    }
    return true;
}

String PathFunctions::stripExtension(const String& fileName, String& extension)
{
    String::size_type foundAt = fileName.rfind('.', fileName.length());

    if (foundAt != String::npos)
    {
        String newFileName = fileName.substr(0, foundAt);
        extension = fileName.substr(foundAt + 1);
        return newFileName;
    }
    return fileName;
}
String PathFunctions::stripExtension(const String& fileName)
{
    String::size_type foundAt = fileName.rfind('.', fileName.length());

    if (foundAt != String::npos)
    {
        return fileName.substr(0, foundAt);
    }
    return fileName;
}

String PathFunctions::fileOrDirectoryName(const String& filePath)
{
    String pathTmp = asGenericPath(filePath);
    String fileName;

    size_t hostDirectoryAt = pathTmp.rfind(TCHAR('/'), pathTmp.length());
    if (hostDirectoryAt != String::npos)
    {
        // Skip the separator char so +1
        fileName = { pathTmp.substr(hostDirectoryAt + 1) };
    }
    return fileName;
}

String PathFunctions::asGenericPath(const String& path)
{
    String pathTmp = path.replaceAllCopy(TCHAR("\\"), TCHAR("/"));
    pathTmp.trimDuplicates(TCHAR('/'));
    return pathTmp;
}
