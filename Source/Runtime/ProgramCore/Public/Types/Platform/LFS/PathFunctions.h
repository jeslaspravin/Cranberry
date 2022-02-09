/*!
 * \file PathFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"
#include "ProgramCoreExports.h"

class PROGRAMCORE_EXPORT PathFunctions
{
private:
    PathFunctions() = default;
public:
    template <typename FirstType, typename LastType> requires StringTypes<FirstType, LastType>
    CONST_EXPR static String combinePath(FirstType&& firstPath, LastType&& lastPath)
    {
        String returnPath(std::forward<FirstType>(firstPath));
        returnPath.append(FS_PATH_SEPARATOR);
        returnPath.append(std::forward<LastType>(lastPath));
        return returnPath;
    }

    template <typename BaseType, typename... Paths> requires StringTypes<BaseType, Paths...>
    CONST_EXPR static String combinePath(BaseType&& basePath, Paths&&... paths)
    {
        String returnPath(std::forward<BaseType>(basePath));
        returnPath.append(FS_PATH_SEPARATOR);
        returnPath.append(combinePath(std::forward<Paths>(paths)...));
        return returnPath;
    }

    static String toRelativePath(const String& absPath, const String& relToPath);
    // To abs path converts relative path to absolute canonical path and replaces any up directory redirectors
    static String toAbsolutePath(const String& relPath, const String& basePath);
    /*
    * Determines if checkDir is sub directory of basePath
    * C:/ABC/DEF/GHI/Some.txt is sub directory of C:/ABC/DEF and not sub directory of C:/ABC/DEF/JKL
    */
    static bool isSubdirectory(const String& checkPath, const String& basePath);

    static String stripExtension(const String& fileName, String& extension);
    static String stripExtension(const String& fileName);
    static String fileOrDirectoryName(const String& filePath);

    static String asGenericPath(const String& path);
};