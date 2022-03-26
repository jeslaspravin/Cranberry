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
#include "Memory/Memory.h"
#include "Types/CompilerDefines.h"
#include "RTTIExampleExports.h"
#include <set>
#include <unordered_map>

#include "TestReflectionGen.gen.h"


class TestConstructionPolicy
{
public:
    // Called for raw allocation deallocation has to be handled by yourself
    template <typename Type>
    static void* allocate() 
    {
        return CBEMemory::memAlloc(sizeof(Type), alignof(Type));
    }
    // Called for new Type(...) allocation if raw allocation failed
    template <typename Type, typename... CtorArgs>
    static Type* newObject(CtorArgs&&... args)
    {
        return new Type(std::forward<CtorArgs>(args)...);
    }

    // Policy available function used only in case of above raw allocation being successful
    template <typename Type, typename... CtorArgs>
    static void preConstruct(void* allocatedPtr, CtorArgs&&... args);
    // Must call the constructor in this function for your custom policy
    template <typename Type, typename... CtorArgs>
    static Type* construct(void* allocatedPtr, CtorArgs&&... args);
    template <typename Type, typename... CtorArgs>
    static void postConstruct(Type* object, CtorArgs&&... args) {}
};

COMPILER_PRAGMA(COMPILER_PUSH_WARNING)
COMPILER_PRAGMA(COMPILER_DISABLE_WARNING(WARN_UNINITIALIZED))

namespace TestNS
{
    class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT, BaseType) BerryObject
    {
        GENERATED_CODES();

    public:
        OVERRIDE_CONSTRUCTION_POLICY(TestConstructionPolicy);

        enum class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT, EmptyStr) ETestEnumClassScoped
        {
            EnumValueZeroth META_ANNOTATE(EmptyStr) = 1,
            EnumValueFirst META_ANNOTATE(EmptyStr) = 2,
            EnumValueSecond META_ANNOTATE(EmptyStr) = 4,
            EnumValueThird META_ANNOTATE(EmptyStr) = 8
        };

        BerryObject() : id(id), strID(strID) {}
        virtual ~BerryObject() = default;
    private:
        friend TestConstructionPolicy;

        int32 id;
        StringID strID;
    };

    class META_ANNOTATE() BerryFirst : public BerryObject
    {
        GENERATED_CODES();

    public:

        META_ANNOTATE()
        std::vector<int32> values;
    };
}

COMPILER_PRAGMA(COMPILER_POP_WARNING)

struct NonReflectType
{
    uint32 value;
};

class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT) BerrySecond : public TestNS::BerryObject
{
    GENERATED_CODES();
public:

    struct META_ANNOTATE() BerrySecondData
    {
        GENERATED_CODES();

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
    
    META_ANNOTATE()
    static void testStaticVoidNoParam(){}
    META_ANNOTATE()
    void testVoidNoParam(){}
    META_ANNOTATE()
    void testConstVoidNoParam() const {}

    BerrySecond() {};
};


template <typename Type, typename... CtorArgs>
void TestConstructionPolicy::preConstruct(void* allocatedPtr, CtorArgs&&... args)
{
    CBEMemory::memZero(allocatedPtr, sizeof(Type));
    TestNS::BerryObject* obj = (TestNS::BerryObject*)(allocatedPtr);
    obj->id = 20;
    obj->strID = STRID("ReflectObj");
}


template <typename Type, typename... CtorArgs>
Type* TestConstructionPolicy::construct(void* allocatedPtr, CtorArgs&&... args)
{
    return new (allocatedPtr)Type(std::forward<CtorArgs>(args)...);
}