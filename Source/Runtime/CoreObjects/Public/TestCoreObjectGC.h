#pragma once


#include "CBEObject.h"

#include "TestCoreObjectGC.gen.h"

struct META_ANNOTATE_API(COREOBJECTS_EXPORT) DataStructAnother
{
    GENERATED_CODES()

    META_ANNOTATE()
    uint32 idx;

    META_ANNOTATE()
    String str;

    META_ANNOTATE()
    CBE::Object* ptr;
};

struct META_ANNOTATE_API(COREOBJECTS_EXPORT) DataStructHashable
{
    GENERATED_CODES()

    META_ANNOTATE()
    uint32 idx;

    META_ANNOTATE()
    String str;

    META_ANNOTATE()
    CBE::Object* ptr;

    bool operator==(const DataStructHashable & other) const
    {
        return idx == other.idx && str == other.str && ptr == other.ptr;
    }
};


template <>
struct COREOBJECTS_EXPORT std::hash<DataStructHashable>
{
    NODISCARD SizeT operator()(const DataStructHashable& keyval) const noexcept 
    {
        return HashUtility::hashAllReturn(keyval.idx, keyval.str, keyval.ptr);
    }
};

struct META_ANNOTATE_API(COREOBJECTS_EXPORT) DataStruct
{
    GENERATED_CODES()

    META_ANNOTATE()
    CBE::Object* objectPtr;

    META_ANNOTATE()
    std::unordered_map<CBE::Object*, uint32> objectPtrToVal;
    
    META_ANNOTATE()
    std::unordered_map<uint32, CBE::Object*> valToObjectPtr;
    
    META_ANNOTATE()
    std::unordered_map<uint32, DataStructAnother> valToStruct;
    
    META_ANNOTATE()
    std::unordered_map<DataStructHashable, uint32> structToVal;
};

class META_ANNOTATE_API(COREOBJECTS_EXPORT) TestCoreObjectGC : public CBE::Object
{
    GENERATED_CODES()

    
    META_ANNOTATE()
    CBE::Object* objectPtr;

    META_ANNOTATE()
    std::unordered_map<CBE::Object*, uint32> objectPtrToVal;
    
    META_ANNOTATE()
    std::unordered_map<uint32, CBE::Object*> valToObjectPtr;
    
    META_ANNOTATE()
    std::unordered_map<uint32, DataStructAnother> valToStruct;
    
    META_ANNOTATE()
    std::unordered_map<DataStructHashable, uint32> structToVal;

    META_ANNOTATE()
    DataStruct ds;
};