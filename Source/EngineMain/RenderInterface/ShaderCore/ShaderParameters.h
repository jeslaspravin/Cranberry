#pragma once
#include "ShaderInputOutput.h"
#include "../../Core/Reflections/MemberField.h"

struct ShaderVertexField;
struct ShaderBufferField;

template<typename ParamType>
struct ShaderParamFieldNode;

template<typename FieldNodeType>
struct ShaderParamInfo;

using ShaderVertexFieldNode = ShaderParamFieldNode<ShaderVertexField>;
using ShaderBufferFieldNode = ShaderParamFieldNode<ShaderBufferField>;

using ShaderVertexParamInfo = ShaderParamInfo<ShaderVertexFieldNode>;
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
};

template<typename OuterType>
struct ShaderBufferTypedField : public ShaderBufferField
{
    ShaderBufferTypedField(const String& pName) : ShaderBufferField(pName) {}

    virtual void* fieldData(OuterType* outerPtr) = 0;
};

template<typename OuterType, typename MemberType>
struct ShaderBufferMemberField : public ShaderBufferTypedField<OuterType>
{
    using ShaderBufferTypedField<OuterType>::bIsArray;
    using ShaderBufferTypedField<OuterType>::bIsStruct;
    using ShaderBufferTypedField<OuterType>::paramInfo;
    using ShaderBufferTypedField<OuterType>::size;
    using ShaderBufferTypedField<OuterType>::stride;

    using FieldPtr = ClassMemberField<false, OuterType, MemberType>;
    FieldPtr memberPtr;

    ShaderBufferMemberField(const String& pName, const FieldPtr& fieldPtr)
        : ShaderBufferTypedField(pName)
        , memberPtr(fieldPtr)
    {
        bIsArray = std::is_array_v<MemberType>;
        size = stride = sizeof(MemberType);
        if (bIsArray)
        {
            stride = sizeof(std::remove_all_extents_t<MemberType>);
        }
    }

    void* fieldData(OuterType* outerPtr) override
    {
        return &memberPtr.get(outerPtr);
    }
};

template<typename OuterType, typename MemberType>
struct ShaderBufferStructField : public ShaderBufferMemberField<OuterType, MemberType>
{
    using ShaderBufferMemberField<OuterType, MemberType>::bIsStruct;
    using ShaderBufferMemberField<OuterType, MemberType>::paramInfo;
    using typename ShaderBufferMemberField<OuterType, MemberType>::FieldPtr;

    ShaderBufferStructField(const String& pName, const FieldPtr& fieldPtr, ShaderBufferParamInfo* pInfo)
        : ShaderBufferMemberField<OuterType, MemberType>::ShaderBufferMemberField(pName, fieldPtr)
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
};


// Parameters info for vertex or buffer inputs
template<typename FieldNodeType>
struct ShaderParamInfo
{
    FieldNodeType startNode;

    virtual uint32 paramStride() = 0;
    virtual void setStride(uint32 newStride) = 0;
};

#define BEGIN_BUFFER_DEFINITION(BufferType) \
struct BufferType##BufferParamInfo : public ShaderBufferParamInfo \
{ \
    typedef BufferType BufferDataType; \
    uint32 stride = sizeof(BufferDataType); \
    uint32 paramStride() { return stride; } \
    void setStride(uint32 newStride) { stride = newStride; }

#define END_BUFFER_DEFINITION() \
}

#define ADD_BUFFER_TYPED_FIELD(FieldName) \
    ShaderBufferMemberField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field = { #FieldName, &BufferDataType::##FieldName }; \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

#define ADD_BUFFER_STRUCT_FIELD(FieldName, FieldType) \
    FieldType##BufferParamInfo FieldName##ParamInfo; \
    ShaderBufferStructField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field = { #FieldName, &BufferDataType::##FieldName##, &##FieldName##ParamInfo }; \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field,& startNode };


#define BEGIN_VERTEX_DEFINITION(VertexType) \
struct VertexType##VertexParamInfo : public ShaderVertexParamInfo \
{ \
    typedef VertexType VertexDataType; \
    uint32 stride = sizeof(VertexDataType); \
    uint32 paramStride() { return stride; } \
    void setStride(uint32 newStride) {}

#define ADD_VERTEX_FIELD(FieldName) \
    ShaderVertexMemberField<VertexDataType, decltype(VertexDataType::##FieldName##)> FieldName##Field = { #FieldName, &VertexDataType::##FieldName, offsetof(VertexDataType, FieldName) }; \
    ShaderVertexFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

#define END_VERTEX_DEFINITION() \
}