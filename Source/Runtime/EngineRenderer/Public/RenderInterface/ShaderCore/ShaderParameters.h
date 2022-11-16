/*!
 * \file ShaderParameters.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Reflections/Fields.h"
#include "String/NameString.h"
#include "RenderInterface/ShaderCore/ShaderInputOutput.h"
#include "Types/Containers/ArrayView.h"
#include "Types/Templates/TypeTraits.h"
#include "Types/Templates/ValueTraits.h"

#include <any>

struct ShaderVertexField;
struct ShaderBufferField;

template <typename ParamType>
struct ShaderParamFieldNode;

template <typename FieldNodeType>
struct ShaderParamInfo;

using ShaderVertexFieldNode = ShaderParamFieldNode<ShaderVertexField>;
using ShaderBufferFieldNode = ShaderParamFieldNode<ShaderBufferField>;

struct ShaderVertexParamInfo;
using ShaderBufferParamInfo = ShaderParamInfo<ShaderBufferFieldNode>;

//////////////////////////////////////////////////////////////////////////
//// Vertex fields
//////////////////////////////////////////////////////////////////////////

struct ENGINERENDERER_EXPORT ShaderVertexField
{
    NameString attributeName{};
    uint32 offset = 0;
    // Location and format will be retrieved from reflection
    uint32 location = 0;
    EShaderInputAttribFormat::Type format = EShaderInputAttribFormat::Undefined;

    ShaderVertexField(const TChar *attribName, uint32 offsetVal);
    ShaderVertexField(const TChar *attribName, uint32 offsetVal, EShaderInputAttribFormat::Type overrideFormat);
};

template <typename OuterType, typename MemberType>
struct ShaderVertexMemberField : public ShaderVertexField
{
    using FieldPtr = ClassMemberField<false, OuterType, MemberType>;
    FieldPtr memberPtr;

    ShaderVertexMemberField(const TChar *pName, const FieldPtr &fieldPtr, uint32 offsetVal)
        : ShaderVertexField(pName, offsetVal)
        , memberPtr(fieldPtr)
    {}

    ShaderVertexMemberField(
        const TChar *pName, const FieldPtr &fieldPtr, uint32 offsetVal, EShaderInputAttribFormat::Type overrideFormat
    )
        : ShaderVertexField(pName, offsetVal, overrideFormat)
        , memberPtr(fieldPtr)
    {}
};

//////////////////////////////////////////////////////////////////////////
//// Buffer fields
//////////////////////////////////////////////////////////////////////////

struct ENGINERENDERER_EXPORT ShaderBufferField
{
    using FieldDecorationFlags = uint8;
    enum EShaderBufferFieldDecorations : FieldDecorationFlags
    {
        IsStruct = 1,
        IsArray = 2,
        IsPointer = 4,
        IsTextureIndex = 8 // Hint to determine if this field is used for Texture indexing
    };
    constexpr static const FieldDecorationFlags INFERRED_DECO_FLAGS
        = EShaderBufferFieldDecorations::IsArray | EShaderBufferFieldDecorations::IsPointer | EShaderBufferFieldDecorations::IsStruct;

    EShaderInputAttribFormat::Type fieldType = EShaderInputAttribFormat::Undefined;
    uint32 offset = 0;
    uint32 stride;
    uint32 size;
    NameString paramName;

    FieldDecorationFlags fieldDecorations = 0;
    ShaderBufferParamInfo *paramInfo = nullptr;

    ShaderBufferField(const TChar *pName, FieldDecorationFlags decorations);

    virtual bool setFieldData(void *outerPtr, const std::any &newValue) const = 0;
    virtual bool setFieldDataArray(void *outerPtr, const std::any &newValuesPtr) const = 0;
    virtual bool setFieldDataArray(void *outerPtr, const std::any &newValue, uint32 arrayIndex) const = 0;
    // returns Pointer to start of element data, if array then pointer to 1st element, If pointer then
    // the pointer itself(not the pointer to pointer) Element size individual element size in array and
    // will be same as type size in non array
    virtual void *fieldData(void *outerPtr, uint32 *outTypeSize, uint32 *outElementSize) const = 0;
    virtual const void *fieldData(const void *outerPtr, uint32 *outTypeSize, uint32 *outElementSize) const = 0;
    // returns Pointer to field, if array then pointer to 1st element, If pointer then pointer to pointer
    virtual void *fieldPtr(void *outerPtr) const = 0;
    FORCE_INLINE bool isIndexAccessible() const
    {
        return ANY_BIT_SET(fieldDecorations, ShaderBufferField::IsArray | ShaderBufferField::IsPointer);
    }
    FORCE_INLINE bool isPointer() const { return BIT_SET(fieldDecorations, ShaderBufferField::IsPointer); }
};

template <typename OuterType>
struct ShaderBufferTypedField : public ShaderBufferField
{
    ShaderBufferTypedField(const TChar *pName, FieldDecorationFlags decorations)
        : ShaderBufferField(pName, decorations)
    {}

    // returns Pointer to start of element data, if array then pointer to 1st element, If pointer then
    // the pointer itself(not the pointer to pointer) Element size individual element size in array and
    // will be same as type size in non array
    virtual const void *fieldData(const OuterType *outerPtr, uint32 *outTypeSize, uint32 *outElementSize) const = 0;
};

template <typename OuterType, typename MemberType>
struct ShaderBufferMemberField : public ShaderBufferTypedField<OuterType>
{
    using typename ShaderBufferTypedField<OuterType>::EShaderBufferFieldDecorations;
    using typename ShaderBufferTypedField<OuterType>::FieldDecorationFlags;
    using ShaderBufferTypedField<OuterType>::fieldDecorations;
    using ShaderBufferTypedField<OuterType>::paramInfo;
    using ShaderBufferTypedField<OuterType>::size;
    using ShaderBufferTypedField<OuterType>::stride;
    using ArrayElementType = IndexableElementType<MemberType>;

    using FieldPtr = ClassMemberField<false, OuterType, MemberType>;
    FieldPtr memberPtr;

    ShaderBufferMemberField(const TChar *pName, const FieldPtr &fieldPtr, FieldDecorationFlags decorations = 0)
        : ShaderBufferTypedField<OuterType>(pName, decorations)
        , memberPtr(fieldPtr)
    {
        fieldDecorations |= std::is_array_v<MemberType> ? EShaderBufferFieldDecorations::IsArray : 0;
        fieldDecorations
            |= std::is_pointer_v<MemberType> ? (EShaderBufferFieldDecorations::IsArray | EShaderBufferFieldDecorations::IsPointer) : 0;

        size = ConditionalValue_v<uint32, uint32, std::is_pointer<MemberType>, 0u, sizeof(MemberType)>;
        stride = sizeof(ArrayElementType);
    }

    constexpr static uint32 totalArrayElements() { return sizeof(MemberType) / sizeof(ArrayElementType); }

    template <Indexable DataType>
    constexpr static IndexableElementType<DataType> *memberDataPtr(DataType &data)
    {
        return &data[0];
    }

    // Do not need to check if DataType is not indexable as compiler prefers concept matched template before generic one
    template <typename DataType>
    constexpr static DataType *memberDataPtr(DataType &data)
    {
        return &data;
    }

    /* ShaderBufferField overrides */
    bool setFieldData(void *outerPtr, const std::any &newValue) const override
    {
        OuterType *castOuterPtr = reinterpret_cast<OuterType *>(outerPtr);
        const MemberType *newValuePtr = std::any_cast<MemberType>(&newValue);

        if (newValuePtr != nullptr)
        {
            memberPtr.set(castOuterPtr, *newValuePtr);
            return true;
        }
        return false;
    }
    bool setFieldDataArray(void *outerPtr, const std::any &newValuesPtr) const override
    {
        OuterType *castOuterPtr = reinterpret_cast<OuterType *>(outerPtr);
        ArrayView<ArrayElementType> const *newValuesViewPtr = std::any_cast<ArrayView<ArrayElementType>>(&newValuesPtr);

        if (ShaderBufferTypedField<OuterType>::isIndexAccessible() && newValuesViewPtr != nullptr && newValuesViewPtr->data() != nullptr)
        {
            const ArrayElementType *newValues = newValuesViewPtr->data();
            if constexpr (std::is_pointer_v<MemberType>)
            {
                if (memberPtr.get(castOuterPtr) != nullptr)
                {
                    ArrayElementType *toValues = memberDataPtr(memberPtr.get(castOuterPtr));
                    memcpy(toValues, newValues, sizeof(ArrayElementType) * newValuesViewPtr->size());
                    return true;
                }
            }
            else
            {
                ArrayElementType *toValues = memberDataPtr(memberPtr.get(castOuterPtr));
                memcpy(toValues, newValues, Math::min(sizeof(MemberType), sizeof(ArrayElementType) * newValuesViewPtr->size()));
                return true;
            }
        }
        return false;
    }
    bool setFieldDataArray(void *outerPtr, const std::any &newValue, uint32 arrayIndex) const override
    {
        OuterType *castOuterPtr = reinterpret_cast<OuterType *>(outerPtr);
        const ArrayElementType *newValuePtr = std::any_cast<ArrayElementType>(&newValue);

        bool bCanSet = false;
        if constexpr (Indexable<MemberType>)
        {
            if constexpr (std::is_pointer_v<MemberType>)
            {
                bCanSet = memberPtr.get(castOuterPtr) != nullptr && newValuePtr != nullptr;
            }
            else
            {
                bCanSet = arrayIndex < totalArrayElements() && newValuePtr != nullptr;
            }
        }
        if (bCanSet)
        {
            ArrayElementType *toValues = memberDataPtr(memberPtr.get(castOuterPtr));
            (toValues)[arrayIndex] = *newValuePtr;
            return true;
        }
        return false;
    }

    void *fieldData(void *outerPtr, uint32 *outTypeSize, uint32 *outElementSize) const override
    {
        if (outTypeSize)
        {
            (*outTypeSize) = sizeof(MemberType);
        }
        if (outElementSize)
        {
            (*outElementSize) = sizeof(ArrayElementType);
        }
        return memberDataPtr(memberPtr.get(reinterpret_cast<OuterType *>(outerPtr)));
    }
    const void *fieldData(const void *outerPtr, uint32 *outTypeSize, uint32 *outElementSize) const override
    {
        if (outTypeSize)
        {
            (*outTypeSize) = sizeof(MemberType);
        }
        if (outElementSize)
        {
            (*outElementSize) = sizeof(ArrayElementType);
        }
        return memberDataPtr(memberPtr.get(reinterpret_cast<const OuterType *>(outerPtr)));
    }
    void *fieldPtr(void *outerPtr) const override { return &memberPtr.get(reinterpret_cast<OuterType *>(outerPtr)); }

    /* ShaderBufferTypedField<OuterType> overrides */
    const void *fieldData(const OuterType *outerPtr, uint32 *outTypeSize, uint32 *outElementSize) const override
    {
        if (outTypeSize)
        {
            (*outTypeSize) = sizeof(MemberType);
        }
        if (outElementSize)
        {
            (*outElementSize) = sizeof(ArrayElementType);
        }
        return memberDataPtr(memberPtr.get(outerPtr));
    }
    /* overrides ends */
};

template <typename OuterType, typename MemberType>
struct ShaderBufferStructField : public ShaderBufferMemberField<OuterType, MemberType>
{
    using typename ShaderBufferMemberField<OuterType, MemberType>::EShaderBufferFieldDecorations;
    using typename ShaderBufferTypedField<OuterType>::FieldDecorationFlags;
    using ShaderBufferMemberField<OuterType, MemberType>::fieldDecorations;
    using ShaderBufferMemberField<OuterType, MemberType>::paramInfo;
    using typename ShaderBufferMemberField<OuterType, MemberType>::FieldPtr;

    ShaderBufferStructField(const TChar *pName, const FieldPtr &fieldPtr, ShaderBufferParamInfo *pInfo, FieldDecorationFlags decorations = 0)
        : ShaderBufferMemberField<OuterType, MemberType>(pName, fieldPtr, decorations)
    {
        paramInfo = pInfo;
        fieldDecorations |= EShaderBufferFieldDecorations::IsStruct;
    }
};

//////////////////////////////////////////////////////////////////////////
//// Nodes form chain to link all of the ShaderParamFields together
//////////////////////////////////////////////////////////////////////////
template <typename ParamType>
struct ShaderParamFieldNode
{
    ParamType *field = nullptr;
    ShaderParamFieldNode *nextNode = nullptr;
    ShaderParamFieldNode *prevNode = nullptr;

    ShaderParamFieldNode() = default;

    ShaderParamFieldNode(ParamType *paramField, ShaderParamFieldNode *headNode)
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

    bool isValid() const { return field != nullptr; }
};

// Parameters info for vertex or buffer inputs
template <typename FieldNodeType>
struct ShaderParamInfo
{
public:
    FieldNodeType startNode;

    virtual uint32 paramStride() const = 0;
    virtual uint32 paramNativeStride() const = 0;
    virtual void setStride(uint32 newStride) = 0;

private:
    class IteratorBase
    {
    protected:
        const FieldNodeType *node;

    public:
        using FieldType = decltype(std::declval<FieldNodeType>().field);
        static_assert(std::is_pointer_v<FieldType>, "Field type must be pointer type");

        /* Iterator traits skipped difference_type as it does not makes sense */
        using value_type = FieldType;
        using reference = FieldType;
        using pointer = FieldType;
        using iterator_category = std::forward_iterator_tag;

        // End constructor, We assume it null rather than iterating through until end
        IteratorBase()
            : node(nullptr)
        {}
        IteratorBase(const ShaderParamInfo *paramInfo)
            : node(&paramInfo->startNode)
        {}
        IteratorBase(const IteratorBase &itr)
            : node(itr.node)
        {}
        IteratorBase(IteratorBase &&itr)
            : node(std::move(itr.node))
        {}
    };

public:
    class ConstIterator : public IteratorBase
    {
    private:
        using IteratorBase::node;

    public:
        using typename IteratorBase::FieldType;

        ConstIterator() = default;
        ConstIterator(const ShaderParamInfo *paramInfo)
            : IteratorBase(paramInfo)
        {}
        ConstIterator(const ConstIterator &itr)
            : IteratorBase(itr)
        {}
        ConstIterator(ConstIterator &&itr)
            : IteratorBase(std::forward(itr))
        {}

        const FieldType operator->() const
        {
            // Okay as FieldType is ptr
            return node ? node->field : nullptr;
        }

        const FieldType operator*() const { return node ? node->field : nullptr; }

        bool operator!=(const ConstIterator &other) const
        {
            const bool thisValid = node != nullptr && node->isValid();
            const bool otherValid = other.node != nullptr && other.node->isValid();
            return thisValid != otherValid || **this != *other;
        }

        ConstIterator &operator++()
        {
            if (node)
            {
                node = node->nextNode;
            }
            return *this;
        }

        ConstIterator operator++(int)
        {
            ConstIterator retVal(*this);
            this->operator++();
            return retVal;
        }
    };

    class Iterator : public IteratorBase
    {
    private:
        using IteratorBase::node;

    public:
        using typename IteratorBase::FieldType;

        Iterator() = default;
        Iterator(const ShaderParamInfo *paramInfo)
            : IteratorBase(paramInfo)
        {}
        Iterator(const Iterator &itr)
            : IteratorBase(itr)
        {}
        Iterator(Iterator &&itr)
            : IteratorBase(std::forward(itr))
        {}

        FieldType operator->() const
        {
            // Okay as FieldType is ptr
            return node ? node->field : nullptr;
        }

        FieldType operator*() const { return node ? node->field : nullptr; }

        bool operator!=(const Iterator &other) const
        {
            const bool thisValid = node != nullptr && node->isValid();
            const bool otherValid = other.node != nullptr && other.node->isValid();
            return thisValid != otherValid || **this != *other;
        }

        Iterator &operator++()
        {
            if (node)
            {
                node = node->nextNode;
            }
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator retVal(*this);
            this->operator++();
            return retVal;
        }
    };

    ConstIterator begin() const { return ConstIterator(this); }

    ConstIterator end() const { return ConstIterator(); }
    Iterator begin() { return Iterator(this); }

    Iterator end() { return Iterator(); }
};

struct ENGINERENDERER_EXPORT ShaderVertexParamInfo : public ShaderParamInfo<ShaderVertexFieldNode>
{
public:
    virtual EShaderInputFrequency::Type inputFrequency() const = 0;
};

#define BEGIN_BUFFER_DEFINITION(BufferType)                                                                                                    \
    struct BufferType##BufferParamInfo final : public ShaderBufferParamInfo                                                                    \
    {                                                                                                                                          \
        typedef BufferType BufferDataType;                                                                                                     \
        uint32 stride = sizeof(BufferDataType);                                                                                                \
        uint32 paramStride() const { return stride; }                                                                                          \
        uint32 paramNativeStride() const { return sizeof(BufferDataType); }                                                                    \
        void setStride(uint32 newStride) { stride = newStride; }

#define END_BUFFER_DEFINITION() }

#define ADD_BUFFER_TYPED_FIELD(FieldName, ...)                                                                                                 \
    ShaderBufferMemberField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field                                          \
        = { TCHAR(#FieldName), &BufferDataType::##FieldName, __VA_ARGS__ };                                                                    \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

// NOTE : Right now supporting : Buffer with any alignment
// but in case of inner struct only with proper alignment with respect to GPU(Alignment correction on
// copying to GPU is only done for first level of variables)
#define ADD_BUFFER_STRUCT_FIELD(FieldName, FieldType, ...)                                                                                     \
    FieldType##BufferParamInfo FieldName##ParamInfo;                                                                                           \
    ShaderBufferStructField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field                                          \
        = { TCHAR(#FieldName), &BufferDataType::##FieldName##, &##FieldName##ParamInfo, __VA_ARGS__ };                                         \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

#define BEGIN_VERTEX_DEFINITION(VertexType, InputFrequency)                                                                                    \
    struct VertexType##VertexParamInfo final : public ShaderVertexParamInfo                                                                    \
    {                                                                                                                                          \
        typedef VertexType VertexDataType;                                                                                                     \
        const EShaderInputFrequency::Type vertexInputFreq = InputFrequency;                                                                    \
        uint32 paramStride() const final { return sizeof(VertexDataType); }                                                                    \
        uint32 paramNativeStride() const final { return sizeof(VertexDataType); }                                                              \
        void setStride(uint32) final {}                                                                                              \
        EShaderInputFrequency::Type inputFrequency() const final { return vertexInputFreq; }

#define ADD_VERTEX_FIELD(FieldName)                                                                                                            \
    ShaderVertexMemberField<VertexDataType, decltype(VertexDataType::##FieldName##)> FieldName##Field                                          \
        = { TCHAR(#FieldName), &VertexDataType::##FieldName, offsetof(VertexDataType, FieldName) };                                            \
    ShaderVertexFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

#define ADD_VERTEX_FIELD_AND_FORMAT(FieldName, OverrideFormat)                                                                                 \
    ShaderVertexMemberField<VertexDataType, decltype(VertexDataType::##FieldName##)> FieldName##Field                                          \
        = { TCHAR(#FieldName), &VertexDataType::##FieldName, offsetof(VertexDataType, FieldName), OverrideFormat };                            \
    ShaderVertexFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

#define END_VERTEX_DEFINITION() }