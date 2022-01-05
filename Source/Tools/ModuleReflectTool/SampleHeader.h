/*!
 * \file SampleHeader.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"
#include <map>
#include <vector>
#include <set>
#include "TestGen.gen.h"

class Vector3D;

#define COMBINE_GENERATED_CODES_internal(A,B,C,D) A##B##C##D
#define COMBINE_GENERATED_CODES(A,B,C,D) COMBINE_GENERATED_CODES_internal(A,B,C,D)
#ifdef __REF_PARSE__
#define META_ANNOTATE(API_EXPORT, ...) __attribute__((annotate( #__VA_ARGS__ )))
#define GENERATED_CODES()
#else
#define META_ANNOTATE(API_EXPORT, ...) API_EXPORT
#define GENERATED_CODES() COMBINE_GENERATED_CODES(HEADER_FILE_ID, _, __LINE__, _GENERATED_CODES)
#endif

#define HEADER_FILE_ID TEST_FILE_H

#define TEST_API __declspec(dllexport)

struct META_ANNOTATE(, TestThis, \
    TEST that, hello world; Ingo\
    Deem \
) TestSt
{
    int32 nothing;
};

enum class META_ANNOTATE(TEST_API, EnumType("Nope")) ETestEnumClassScoped
{
    EnumValueZeroth META_ANNOTATE(, EnumType("Zeroth")),
    EnumValueFirst META_ANNOTATE(, EnumType("First")),
    EnumValueSecond META_ANNOTATE(, EnumType("Second")),
    EnumValueThird META_ANNOTATE(, EnumType("Third"))
};

namespace ETestEnumGlobalScoped
{
    enum META_ANNOTATE(TEST_API, EnumType("Nope")) Type
    {
        ValueOne META_ANNOTATE(, EnumType("One")) = 1,
        ValueTwo,
        ValueThree,
        ValueFour
    };
}

struct META_ANNOTATE(, HelloSir) Anarchy
{
    GENERATED_CODES()

    AChar test;
    AChar* testPtr;
    META_ANNOTATE(, Deprecate)
    const AChar testConst;
    const AChar* testPtrToConst;
    const AChar& testInvalidRef;
    static AChar testStatic;
};

class META_ANNOTATE(TEST_API, MarkBase) MyClass
{
private:
    using InAnarchy = Anarchy;
    friend InAnarchy;
public:
    int32 field = 34;

    MyClass();
    MyClass(int32 inField);

    operator int32() const;
    MyClass& operator=(int32 value);

    META_ANNOTATE(, Deprecate)
    virtual void method(const AChar *ch, const int32 & idxRef, Anarchy& outIdx, int32 inNum, int32 *optionalNum) const = 0;

    static const int static_field;
    static int32 const* const static_method(std::map<int32, Anarchy> outMap, const std::vector<int32>& inList, std::pair<int32, Vector3D>& testPair, std::set<int32>& setTest);
};

int32 global_method();
namespace TestNSL1
{
    struct L1Struct : public Anarchy
    {
        int32 abc;
        float del;
        int32 list[50];
    };

    class META_ANNOTATE(, ChildClass) MyChildClass : public MyClass
    {
    public:
        void method(const AChar * ch, const int32 & idxRef, Anarchy & outIdx, int32 inNum, int32 * optionalNum) const override;
    };

    namespace TestNSL2
    {
        struct META_ANNOTATE(TEST_API, NothingToSeeHere("HeHe")) L2Struct
        {
            int32 abcl2;
            float dell2;
            int32 listl2[32];
        };

        class META_ANNOTATE(, GrandChildClass) MyGrandChildClass : public TestNSL1::MyChildClass
        {
        public:
            int64 testUnknownType;
            L2Struct l2Struct;
            L2Struct* l2Struct2;
            const L2Struct* l2Struct3;
            std::map<int32, L2Struct> idxTol2;
            std::map<int32, L2Struct*>* idxTol3;
            const std::map<int32, L2Struct*>* idxTol4;
        private:
            void method(const AChar * ch, const int32 & idxRef, Anarchy & outIdx, int32 inNum, int32 * optionalNum) const final;

            auto getUnknown() { return testUnknownType; }
        };

        int32 global_namespaced_method();
    }
}