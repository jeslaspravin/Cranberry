#pragma once
#include "ShaderInputOutput.h"
#include "../../Core/Reflections/MemberField.h"

#include <any>

struct ShaderVertexField;
struct ShaderBufferField;

template<typename ParamType>
struct ShaderParamFieldNode;

template<typename FieldNodeType>
struct ShaderParamInfo;

using ShaderVertexFieldNode = ShaderParamFieldNode<ShaderVertexField>;
using ShaderBufferFieldNode = ShaderParamFieldNode<ShaderBufferField>;

struct ShaderVertexParamInfo;
using ShaderBufferParamInfo = ShaderParamInfo<ShaderBufferFieldNode>;

//////////////////////////////////////////////////////////////////////////
//// Vertex fields
//////////////////////////////////////////////////////////////////////////

struct ShaderVertexField
{
    String attributeName = "";
    uint32 offset = 0;
    // Location and format will be retrieved from reflection
    uint32 location = 0;
    EShaderInputAttribFormat::Type format = EShaderInputAttribFormat::Undefined;

    ShaderVertexField(const String& attribName, const uint32& offsetVal);
    ShaderVertexField(const String& attribName, const uint32& offsetVal, EShaderInputAttribFormat::Type overrideFormat);
};

template<typename OuterType, typename MemberType>
struct ShaderVertexMemberField : public ShaderVertexField
{
    using FieldPtr = ClassMemberField<false, OuterType, MemberType>;
    FieldPtr memberPtr;

    ShaderVertexMemberField(const String& pName, const FieldPtr& fieldPtr, const uint32& offsetVal)
        : ShaderVertexField(pName, offsetVal)
        , memberPtr(fieldPtr)
    {}

    ShaderVertexMemberField(const String& pName, const FieldPtr& fieldPtr, const uint32& offsetVal, EShaderInputAttribFormat::Type overrideFormat)
        : ShaderVertexField(pName, offsetVal, overrideFormat)
        , memberPtr(fieldPtr)
    {}
};

//////////////////////////////////////////////////////////////////////////
//// Buffer fields
//////////////////////////////////////////////////////////////////////////

struct ShaderBufferField
{
    EShaderInputAttribFormat::Type fieldType = EShaderInputAttribFormat::Undefined;
    uint32 offset = 0;
    uint32 stride;
    uint32 size;
    String paramName;

    bool bIsStruct = false;
    bool bIsArray = false;
    ShaderBufferParamInfo* paramInfo = nullptr;

    ShaderBufferField(const String& pName);

    virtual bool setFieldData(void* outerPtr, const std::any& newValue) const = 0;
    virtual bool setFieldDataArray(void* outerPtr, const std::any& newValuesPtr) const = 0;
    virtual bool setFieldDataArray(void* outerPtr, const std::any& newValue, uint32 arrayIndex) const = 0;
    virtual void* fieldData(uint32& typeSize, void* outerPtr) const = 0;
};

template<typename OuterType>
struct ShaderBufferTypedField : public ShaderBufferField
{
    ShaderBufferTypedField(const String& pName) : ShaderBufferField(pName) {}

    virtual const void* fieldData(uint32& typeSize, const OuterType* outerPtr) const = 0;
};

template<typename OuterType, typename MemberType>
struct ShaderBufferMemberField : public ShaderBufferTypedField<OuterType>
{
    using ShaderBufferTypedField<OuterType>::bIsArray;
    using ShaderBufferTypedField<OuterType>::bIsStruct;
    using ShaderBufferTypedField<OuterType>::paramInfo;
    using ShaderBufferTypedField<OuterType>::size;
    using ShaderBufferTypedField<OuterType>::stride;
    using ArrayType = std::remove_all_extents_t<MemberType>;

    using FieldPtr = ClassMemberField<false, OuterType, MemberType>;
    FieldPtr memberPtr;

    ShaderBufferMemberField(const String& pName, const FieldPtr& fieldPtr)
        : ShaderBufferTypedField<OuterType>(pName)
        , memberPtr(fieldPtr)
    {
        bIsArray = std::is_array_v<MemberType>;
        size = stride = sizeof(MemberType);
        if (bIsArray)
        {
            stride = sizeof(ArrayType);
        }
    }

    constexpr static uint32 totalArrayElements()
    {
        // Right now supporting only one dimension
        return uint32(std::extent_v<MemberType, 0>);
    }

    template <typename DataType>
    constexpr static std::enable_if_t<std::is_array_v<DataType>, DataType>* memberDataPtr(DataType& data)
    {
        return &data[0];
    }

    template <typename DataType>
    constexpr static std::enable_if_t<std::negation_v<std::is_array<DataType>>, DataType>* memberDataPtr(DataType& data)
    {
        return &data;
    }

    bool setFieldData(void* outerPtr, const std::any& newValue) const override
    {
        OuterType* castOuterPtr = reinterpret_cast<OuterType*>(outerPtr);
        const MemberType* newValuePtr = std::any_cast<MemberType>(&newValue);

        if (newValuePtr != nullptr)
        {
            memberPtr.set(castOuterPtr, *newValuePtr);
            return true;
        }
        return false;
    }

    bool setFieldDataArray(void* outerPtr, const std::any& newValuesPtr) const override
    {
        OuterType* castOuterPtr = reinterpret_cast<OuterType*>(outerPtr);
        MemberType *const*newValuesPtrPtr = std::any_cast<MemberType*>(&newValuesPtr);

        if (bIsArray && newValuesPtrPtr != nullptr && *newValuesPtrPtr != nullptr)
        {
            MemberType* newValues = *newValuesPtrPtr;
            MemberType* toValues = memberDataPtr(memberPtr.get(castOuterPtr));

            memcpy(toValues, newValues, sizeof(MemberType));
            return true;
        }
        return false;
    }

    bool setFieldDataArray(void* outerPtr, const std::any& newValue, uint32 arrayIndex) const override
    {
        OuterType* castOuterPtr = reinterpret_cast<OuterType*>(outerPtr);
        const MemberType* newValuesPtr = std::any_cast<MemberType>(&newValue);

        if (bIsArray && arrayIndex < totalArrayElements() && newValuesPtr != nullptr)
        {
            MemberType* toValues = memberDataPtr(memberPtr.get(castOuterPtr));

            toValues[arrayIndex] = *newValuesPtr;

            return true;
        }
        return false;
    }

    void* fieldData(uint32& typeSize, void* outerPtr) const override
    {
        typeSize = sizeof(MemberType);
        return memberDataPtr(memberPtr.get(reinterpret_cast<OuterType*>(outerPtr)));
    }

    const void* fieldData(uint32& typeSize, const OuterType* outerPtr) const override
    {
        typeSize = sizeof(MemberType);
        return memberDataPtr(memberPtr.get(outerPtr));
    }
};

template<typename OuterType, typename MemberType>
struct ShaderBufferStructField : public ShaderBufferMemberField<OuterType, MemberType>
{
    using ShaderBufferMemberField<OuterType, MemberType>::bIsStruct;
    using ShaderBufferMemberField<OuterType, MemberType>::paramInfo;
    using typename ShaderBufferMemberField<OuterType, MemberType>::FieldPtr;

    ShaderBufferStructField(const String& pName, const FieldPtr& fieldPtr, ShaderBufferParamInfo* pInfo)
        : ShaderBufferMemberField<OuterType, MemberType>(pName, fieldPtr)
    {
        paramInfo = pInfo;
        bIsStruct = true;
    }
};

//////////////////////////////////////////////////////////////////////////
//// Nodes form chain to link all of the ShaderParamFields together
//////////////////////////////////////////////////////////////////////////
template<typename ParamType>
struct ShaderParamFieldNode
{
    ParamType* field = nullptr;
    ShaderParamFieldNode* nextNode = nullptr;
    ShaderParamFieldNode* prevNode = nullptr;

    ShaderParamFieldNode() = default;

    ShaderParamFieldNode(ParamType* paramField, ShaderParamFieldNode* headNode)
        : nextNode(nullptr)
        , prevNode(nullptr)
    {
        prevNode = headNode;
        while (prevNode->nextNode != nullptr)
        {
            prevNode = prevNode->nextNode;
        }
        prevNode->nextNode = this;
        prevNode->field = paramField;
    }

    bool isValid() const
    {
        return field != nullptr;
    }
};


// Parameters info for vertex or buffer inputs
template<typename FieldNodeType>
struct ShaderParamInfo
{
public:
    FieldNodeType startNode;
    // TODO(Jeslas) : change param info to use ranged for loop rather than manual while based iteration
    //FieldNodeType* endNode;
    //ShaderParamInfo()
    //{
    //    FieldNodeType* node = &startNode;
    //    while (node->isValid())
    //    {
    //        node = node->nextNode;
    //    }
    //    endNode = node;
    //}

    virtual uint32 paramStride() const = 0;
    virtual void setStride(uint32 newStride) = 0;
};

struct ShaderVertexParamInfo : public ShaderParamInfo<ShaderVertexFieldNode>
{
public:
    virtual EShaderInputFrequency::Type inputFrequency() const = 0;
};

#define BEGIN_BUFFER_DEFINITION(BufferType) \
struct BufferType##BufferParamInfo final : public ShaderBufferParamInfo \
{ \
    typedef BufferType BufferDataType; \
    uint32 stride = sizeof(BufferDataType); \
    uint32 paramStride() const { return stride; } \
    void setStride(uint32 newStride) { stride = newStride; }

#define END_BUFFER_DEFINITION() \
}

#define ADD_BUFFER_TYPED_FIELD(FieldName) \
    ShaderBufferMemberField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field = { #FieldName, &BufferDataType::##FieldName }; \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

// NOTE : Right now only supporting inner struct with proper alignment with respect to GPU(Alignment correction on copying to GPU is only done for first level of variables)  
#define ADD_BUFFER_STRUCT_FIELD(FieldName, FieldType) \
    FieldType##BufferParamInfo FieldName##ParamInfo; \
    ShaderBufferStructField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field = { #FieldName, &BufferDataType::##FieldName##, &##FieldName##ParamInfo }; \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field, &startNode };


#define BEGIN_VERTEX_DEFINITION(VertexType, InputFrequency) \
struct VertexType##VertexParamInfo final : public ShaderVertexParamInfo \
{ \
    typedef VertexType VertexDataType; \
    const uint32 stride = sizeof(VertexDataType); \
    const EShaderInputFrequency::Type vertexInputFreq =  InputFrequency; \
    uint32 paramStride() const final { return stride; } \
    void setStride(uint32 newStride) final {} \
    EShaderInputFrequency::Type inputFrequency() const final { return  vertexInputFreq; }

#define ADD_VERTEX_FIELD(FieldName) \
    ShaderVertexMemberField<VertexDataType, decltype(VertexDataType::##FieldName##)> FieldName##Field = { #FieldName, &VertexDataType::##FieldName, offsetof(VertexDataType, FieldName) }; \
    ShaderVertexFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

#define ADD_VERTEX_FIELD_AND_FORMAT(FieldName, OverrideFormat) \
    ShaderVertexMemberField<VertexDataType, decltype(VertexDataType::##FieldName##)> FieldName##Field = { #FieldName, &VertexDataType::##FieldName, offsetof(VertexDataType, FieldName), OverrideFormat }; \
    ShaderVertexFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

#define END_VERTEX_DEFINITION() \
}