/*!
 * \file ObjectPathHelpers.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoreObjectsExports.h"
#include "String/String.h"

class COREOBJECTS_EXPORT ObjectPathHelper
{
private:
    ObjectPathHelper() = default;

    FORCE_INLINE static StringView getOuterPathAndObjectName(StringView &outObjectName, StringView objectPath);

public:
    // Object paths will be "RootObjName:OuterMostObjName/OuterObjName/ObjName"
    // Why? This will in long term will be helpful to manage all objects under one root/sub-objects and
    // traversing the object tree will become so much easier
    CONST_EXPR static const TChar ObjectObjectSeparator = TCHAR('/');
    CONST_EXPR static const TChar RootObjectSeparator = TCHAR(':');

    static String computeFullPath(const cbe::Object *object);
    static String computeFullPath(StringView objectName, const cbe::Object *outerObj);
    static String computeObjectPath(const cbe::Object *object, const cbe::Object *stopAt);

    static String getFullPath(StringView objectName, const cbe::Object *outerObj);

    static StringView getPackagePath(StringView objFullPath);
    /**
     * ObjectPathHelper::getPathComponents
     *
     * Access: public static
     *
     * @param String & outOuterObjectPath - Outer Object path without root/package path
     * @param const TChar * objFullPath - In full path
     *
     * @return String - Package name/path
     */
    static StringView getPathComponents(StringView &outOuterObjectPath, StringView &outObjectName, StringView objFullPath);
    static String combinePathComponents(StringView packagePath, StringView outerObjectPath, StringView objectName);

    static StringView getObjectName(StringView objPath);

    // Just helper to split package host path and package's name
    static StringView splitPackageNameAndPath(StringView &outName, StringView objPath);

    static String packagePathFromFilePath(const String &filePath, const String &contentDir);

    static bool isValidPackageName(StringView packageName);
    static String getValidPackageName(StringView packageName);
};
