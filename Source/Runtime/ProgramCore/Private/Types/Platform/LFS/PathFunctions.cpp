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
#include "String/String.h"

#include <filesystem>

String PathFunctions::toRelativePath(const String &absPath, const String &relToPath)
{
    std::filesystem::path absolutePath(absPath.getChar());
    std::filesystem::path relativeToPath(relToPath.getChar());
    fatalAssertf(relativeToPath.is_absolute(), "Relative to path {} must be absolute path", relToPath);
    if (absolutePath.is_relative())
    {
        return absPath;
    }
    // Should we implement a toRelativePath platform independent? std::filesystem::relative does that
    // under the hood anyways
    std::error_code errorCode;
    std::filesystem::path relPath = std::filesystem::relative(absolutePath, relativeToPath, errorCode);
    fatalAssertf(
        errorCode.value() == 0, "Error {} when making [{}] as relative to {}", UTF8_TO_TCHAR(errorCode.message().c_str()), absPath, relToPath
    );
    return WCHAR_TO_TCHAR(relPath.c_str());
}

String PathFunctions::toAbsolutePath(const String &relPath, const String &basePath)
{
    // Check if path is relative
    {
        std::filesystem::path relativePath(relPath.getChar());
        if (relativePath.is_absolute())
        {
            return relPath;
        }
    }

    String absPath(PathFunctions::combinePath(basePath, relPath));
    absPath.replaceAll(TCHAR("\\"), TCHAR("/"));
    // Split each path elements
    const std::vector<StringView> pathElems{ String::split(absPath, TCHAR("/")) };
    std::vector<StringView> sanitizedPathElems;
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
    return String::join(sanitizedPathElems.crbegin(), sanitizedPathElems.crend(), TCHAR("/"));
}

bool PathFunctions::isSubdirectory(const String &checkPath, const String &basePath)
{
    String genericCheckPath = asGenericPath(checkPath);
    String genericBasePath = asGenericPath(basePath);
    const std::vector<StringView> checkPathElems{ String::split(genericCheckPath, TCHAR("/")) };
    const std::vector<StringView> basePathElems{ String::split(genericBasePath, TCHAR("/")) };

    // If basePath folder count is larger or equal to checkPath then it means basPath can not fit into
    // checkPath. checkPath can never be subdir of basePath
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

bool PathFunctions::isRelativePath(const String &checkPath)
{
    std::filesystem::path relativePath(checkPath.getChar());
    return relativePath.is_relative();
}

String PathFunctions::stripExtension(String &extension, const String &fileName)
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
String PathFunctions::stripExtension(const String &fileName)
{
    String::size_type foundAt = fileName.rfind('.', fileName.length());

    if (foundAt != String::npos)
    {
        return fileName.substr(0, foundAt);
    }
    return fileName;
}

String PathFunctions::fileOrDirectoryName(const String &filePath)
{
    String pathTmp = asGenericPath(filePath);
    size_t hostDirectoryAt = pathTmp.rfind(TCHAR('/'), pathTmp.length());
    if (hostDirectoryAt != String::npos)
    {
        // Skip the separator char so +1
        return { pathTmp.substr(hostDirectoryAt + 1) };
    }
    return filePath;
}

String PathFunctions::splitFileAndDirectory(String &outFileName, const String &filePath)
{
    String pathTmp = asGenericPath(filePath);
    size_t hostDirectoryAt = pathTmp.rfind(TCHAR('/'), pathTmp.length());
    if (hostDirectoryAt != String::npos)
    {
        // Skip the separator char so +1
        outFileName = { pathTmp.substr(hostDirectoryAt + 1) };
        return { pathTmp.substr(0, hostDirectoryAt) };
    }
    return pathTmp;
}

String PathFunctions::parentDirectory(const String &filePath)
{
    String pathTmp = asGenericPath(filePath);
    size_t hostDirectoryAt = pathTmp.rfind(TCHAR('/'), pathTmp.length());
    if (hostDirectoryAt != String::npos)
    {
        return { pathTmp.substr(0, hostDirectoryAt) };
    }
    return {};
}

String PathFunctions::asGenericPath(const String &path)
{
    String pathTmp = path.replaceAllCopy(TCHAR("\\"), TCHAR("/"));
    pathTmp.trimDuplicates(TCHAR('/'));
    if (pathTmp.endsWith(TCHAR('/')))
    {
        pathTmp.eraseR(1);
    }
    return pathTmp;
}
