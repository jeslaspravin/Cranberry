/*!
 * \file BasicPackagedObject.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObject.h"
#include "RTTIExampleExports.h"
#include "CBEObjectHelpers.h"
#include "String/String.h"

#include "BasicPackagedObject.gen.h"

class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT) BasicPackagedObject : public CBE::Object
{
    GENERATED_CODES()
public:
    BasicPackagedObject()
    {
        if (getOuter() && getOuter()->getType() != staticType())
        {
            inner = CBE::create<BasicPackagedObject>(TCHAR("SubObject"), this);
        }
    }
    std::map<uint32, String> idxToStr;
    float dt;
    StringID id;
    String nameVal;
    BasicPackagedObject *interLinked;
    BasicPackagedObject *inner;

    ObjectArchive &serialize(ObjectArchive &ar)
    {
        ar << idxToStr;
        ar << dt;
        ar << id;
        ar << nameVal;
        ar << interLinked;
        ar << inner;
        return ar;
    }
};