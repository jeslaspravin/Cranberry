/*!
 * \file ObjectPathHelpers.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoreObjectsExports.h"
#include "String/TCharString.h"

class COREOBJECTS_EXPORT ObjectPathHelper
{
private:
    ObjectPathHelper() = default;

    FORCE_INLINE static String getOuterPathAndObjectName(String &outObjectName, const TChar *objectPath);

public:
    // Object paths will be "RootObjName:OuterMostObjName/OuterObjName/ObjName"
    // Why? This will in long term will be helpful to manage all objects under one root/sub-objects and
    // traversing the object tree will become so much easier
    CONST_EXPR static const TChar ObjectObjectSeparator = '/';
    CONST_EXPR static const TChar RootObjectSeparator = ':';

    static String getFullPath(const cbe::Object *object);
    static String getFullPath(const TChar *objectName, const cbe::Object *outerObj);
    static String getObjectPath(const cbe::Object *object, const cbe::Object *stopAt);

    static String getPackagePath(const TChar *objFullPath);
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
    static String getPathComponents(String &outOuterObjectPath, String &outObjectName, const TChar *objFullPath);
    static String combinePathComponents(const String &packagePath, const String &outerObjectPath, const String &objectName);

    // Just helper to split package host path and package's name
    static String splitPackageNameAndPath(String &outName, const TChar *path);

    static String packagePathFromFilePath(const String &filePath, const String &contentDir);

    static bool isValidPackageName(const String &packageName);
    static String getValidPackageName(const String &packageName);
};
