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
#include "InterfaceExample.h"
#include "String/String.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Serialization/ObjectSerializationHelpers.h"

#include "BasicPackagedObject.gen.h"

struct META_ANNOTATE() SimpleStruct
{
    GENERATED_CODES()

public:
    META_ANNOTATE()
    float a = -1.0f;
    META_ANNOTATE()
    int32 b = -1;
    META_ANNOTATE()
    String testStr = "Default value";
};

// Example custom serialization version check at BasicPackagedObject
constexpr inline const uint32 BASICPACKAGEDOBJ_SERIALIZER_VERSION = 1;
constexpr inline const uint32 BASICPACKAGEDOBJ_SERIALIZER_CUTOFF_VERSION = 0;

class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT) BasicPackagedObject
    : public cbe::Object
    , public IInterfaceExample
    , public IInterfaceExample2
{
    GENERATED_CODES()
public:
    std::map<uint32, String> idxToStr;
    float dt;
    StringID id;
    String nameVal;
    SimpleStruct structData;
    BasicPackagedObject *interLinked;
    BasicPackagedObject *inner;

    BasicPackagedObject()
    {
        if (getOuter() && getOuter()->getType() != staticType())
        {
            inner = cbe::create<BasicPackagedObject>(TCHAR("SubObject"), this);
        }
    }

    ObjectArchive &serialize(ObjectArchive &ar) override
    {
        uint32 packageVersion = BASICPACKAGEDOBJ_SERIALIZER_VERSION;
        if (ar.isLoading())
        {
            packageVersion = ar.getCustomVersion(uint32(staticType()->name));
            if (packageVersion < BASICPACKAGEDOBJ_SERIALIZER_CUTOFF_VERSION)
            {
                debugAssertf(
                    packageVersion >= BASICPACKAGEDOBJ_SERIALIZER_CUTOFF_VERSION, "Unsupported serialization version for object %s of class %s",
                    getObjectData().name, staticType()->nameString
                );
                return ar;
            }
        }
        else
        {
            ar.setCustomVersion(uint32(staticType()->name), BASICPACKAGEDOBJ_SERIALIZER_VERSION);
        }

        ar << idxToStr;
        ar << dt;
        ar << id;
        ar << nameVal;
        ar << interLinked;
        ar << inner;

        if (packageVersion >= BASICPACKAGEDOBJ_SERIALIZER_VERSION)
        {
            ObjectSerializationHelpers::serializeStructFields(structData, ar);
        }
        return ar;
    }

    void onPostLoad() override;
    void onConstructed() override;
    void exampleFunc() const override;
};

class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT) BasicFieldSerializedObject
    : public cbe::Object
    , public IInterfaceExample
    , public IInterfaceExample2
{
    GENERATED_CODES()
public:
    // Overriding Allocator slot count to 8
    constexpr static const uint32 AllocSlotCount = 8;
    constexpr static const bool bOnlySelectedFields = false;

public:
    META_ANNOTATE()
    std::map<uint32, std::map<String, uint32>> idxToStr;

    META_ANNOTATE()
    float dt;

    META_ANNOTATE()
    StringID id;

    META_ANNOTATE()
    String nameVal;

    META_ANNOTATE()
    SimpleStruct structData;

    META_ANNOTATE()
    BasicPackagedObject *interLinked;

    META_ANNOTATE()
    BasicPackagedObject *inner;

    BasicFieldSerializedObject()
    {
        if (getOuter() && getOuter()->getType() != staticType())
        {
            inner = cbe::create<BasicPackagedObject>(TCHAR("SubObject"), this);
        }
    }

    ObjectArchive &serialize(ObjectArchive &ar) override
    {
        if constexpr (bOnlySelectedFields)
        {
            std::unordered_set<StringID> selectedFields{ TCHAR("dt"), TCHAR("id") };
            return ObjectSerializationHelpers::serializeOnlyFields(this, ar, selectedFields);
        }
        else
        {
            return ObjectSerializationHelpers::serializeAllFields(this, ar);
        }
    }

    void onPostLoad() override;
    void onConstructed() override;
    void exampleFunc() const override;
};