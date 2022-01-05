/*!
 * \file TestReflectionGen.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ReflectionMacros.h"
#include "CoreObjectsExports.h"
#include <set>
#include <unordered_map>

#include "TestReflectionGen.gen.h"

namespace TestNS
{
    class META_ANNOTATE_API(COREOBJECTS_EXPORT, BaseType) BerryObject
    {
        GENERATED_CODES()

    public:
        enum class META_ANNOTATE_API(COREOBJECTS_EXPORT, EmptyStr) ETestEnumClassScoped
        {
            EnumValueZeroth META_ANNOTATE(EmptyStr) = 1,
            EnumValueFirst META_ANNOTATE(EmptyStr) = 2,
            EnumValueSecond META_ANNOTATE(EmptyStr) = 4,
            EnumValueThird META_ANNOTATE(EmptyStr) = 8
        };
    };

    class META_ANNOTATE() BerryFirst : public BerryObject
    {
        GENERATED_CODES()
    };
}

struct NonReflectType
{
    uint32 value;
};

class META_ANNOTATE_API(COREOBJECTS_EXPORT) BerrySecond : public TestNS::BerryObject
{
    GENERATED_CODES()
public:

    struct META_ANNOTATE() BerrySecondData
    {
        GENERATED_CODES()

        META_ANNOTATE()
        uint32 value;
    };

    META_ANNOTATE()
    BerrySecondData reflectedStruct;

    META_ANNOTATE()
    TestNS::BerryObject::ETestEnumClassScoped options;

    META_ANNOTATE()
    std::map<uint64, BerrySecondData> idxToBerrySec;
    
    META_ANNOTATE()
    std::vector<BerrySecondData> berries;

    META_ANNOTATE()
    std::set<BerrySecondData> uniqueBerries;

    META_ANNOTATE()
    std::unordered_map<uint64, BerrySecondData> idxToBerrySec2;

    META_ANNOTATE()
    std::pair<uint32, TestNS::BerryObject*> idxToObjext;

    META_ANNOTATE()
    void testThisFunction(std::pair<uint32, TestNS::BerryObject*>& aValue, const std::unordered_map<uint64, BerrySecondData*>& bValue, uint32 values){}
    
    META_ANNOTATE()
    void testConstFunction(std::vector<std::pair<uint32, TestNS::BerryObject*>>& aValues, const std::unordered_map<uint64, TestNS::BerryObject*>& bValue, uint32 values) const{}
    
    META_ANNOTATE()
    static std::set<std::pair<uint32, TestNS::BerryObject*>> testStaticFunc(std::vector<std::pair<uint32, TestNS::BerryObject*>>*& aValues
        , const std::unordered_map<uint64, TestNS::BerryObject*>& bValue, uint32 values)
    {
        return {};
    }
};
