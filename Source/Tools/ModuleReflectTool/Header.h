#pragma once

#include "Types/CoreTypes.h"

#ifdef __REF_PARSE__
#define META_ANNOTATE(API_EXPORT, ...) __attribute__((annotate( #__VA_ARGS__ )))
#else
#define META_ANNOTATE(API_EXPORT, ...) API_EXPORT
#endif

#define TEST_API __declspec(dllexport)

enum class META_ANNOTATE(TEST_API, EnumType("Nope")) ETestEnumClassScoped
{
    EnumValueZeroth META_ANNOTATE(, EnumType("Zeroth")),
    EnumValueFirst META_ANNOTATE(, EnumType("First")),
    EnumValueSecond META_ANNOTATE(, EnumType("Second")),
    EnumValueThird META_ANNOTATE(, EnumType("Third")),
};

namespace ETestEnumGlobalScoped
{
    enum META_ANNOTATE(TEST_API, EnumType("Nope")) Type
    {
        ValueOne META_ANNOTATE(, EnumType("One")) = 1,
        ValueTwo,
        ValueThree,
        ValueFour,
    };
}

struct META_ANNOTATE(, HelloSir) Anarchy
{
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
    virtual void method(const AChar *ch, const int32 & idxRef, int32& outIdx, int32 inNum, int32 *optionalNum) const = 0;

    static const int static_field;
    static int32 static_method();
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
        void method(const AChar * ch, const int32 & idxRef, int32 & outIdx, int32 inNum, int32 * optionalNum) const override;
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
        private:
            void method(const AChar * ch, const int32 & idxRef, int32 & outIdx, int32 inNum, int32 * optionalNum) const final;

            auto getUnknown() { return testUnknownType; }
        };

        int32 global_namespaced_method();
    }
}