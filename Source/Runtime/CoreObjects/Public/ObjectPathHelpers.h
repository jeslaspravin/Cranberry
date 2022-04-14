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

public:
    // Object paths will be "RootObjName:OuterMostObjName/OuterObjName/ObjName"
    // Why? This will in long term will be helpful to manage all objects under one root/sub-objects and
    // traversing the object tree will become so much easier
    CONST_EXPR static const TChar ObjectObjectSeparator = '/';
    CONST_EXPR static const TChar RootObjectSeparator = ':';

    static String getFullPath(const CBE::Object *object);
    static String getFullPath(const TChar *objectName, const CBE::Object *outerObj);
};