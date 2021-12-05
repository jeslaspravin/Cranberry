#pragma once
#include "RenderInterface/ShaderCore/ShaderInputOutput.h"
#include "Reflections/MemberField.h"
#include "Types/Containers/ArrayView.h"
#include "Types/Traits/ValueTraits.h"
#include "Types/Traits/TypeTraits.h"

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

struct ENGINERENDERER_EXPORT ShaderVertexField
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

struct ENGINERENDERER_EXPORT ShaderBufferField
{
    enum ShaderBufferFieldDecorations : uint8
    {
        IsStruct = 1,
        IsArray = 2,
        IsPointer = 4
    };

    EShaderInputAttribFormat::Type fieldType = EShaderInputAttribFormat::Undefined;
    uint32 offset = 0;
    uint32 stride;
    uint32 size;
    String paramName;

    uint8 fieldDecorations = 0;
    ShaderBufferParamInfo* paramInfo = nullptr;

    ShaderBufferField(const String& pName);

    virtual bool setFieldData(void* outerPtr, const std::any& newValue) const = 0;
    virtual bool setFieldDataArray(void* outerPtr, const std::any& newValuesPtr) const = 0;
    virtual bool setFieldDataArray(void* outerPtr, const std::any& newValue, uint32 arrayIndex) const = 0;
    // returns Pointer to start of element data, if array then pointer to 1st element, If pointer then the pointer itself(not the pointer to pointer)
    // Element size individual element size in array and will be same as type size in non array
    virtual void* fieldData(void* outerPtr, uint32* typeSize, uint32* elementSize) const = 0;
    // returns Pointer to field, if array then pointer to 1st element, If pointer then pointer to pointer
    virtual void* fieldPtr(void* outerPtr) const = 0;
    FORCE_INLINE bool isIndexAccessible() const
    {
        return ANY_BIT_SET(fieldDecorations, ShaderBufferField::IsArray | ShaderBufferField::IsPointer);
    }
    FORCE_INLINE bool isPointer() const
    {
        return BIT_SET(fieldDecorations, ShaderBufferField::IsPointer);
    }
};

template<typename OuterType>
struct ShaderBufferTypedField : public ShaderBufferField
{
    ShaderBufferTypedField(const String& pName) : ShaderBufferField(pName) {}

    // returns Pointer to start of element data, if array then pointer to 1st element, If pointer then the pointer itself(not the pointer to pointer)
    // Element size individual element size in array and will be same as type size in non array
    virtual const void* fieldData(const OuterType* outerPtr, uint32* typeSize, uint32* elementSize) const = 0;
};

template<typename OuterType, typename MemberType>
struct ShaderBufferMemberField : public ShaderBufferTypedField<OuterType>
{
    using ShaderBufferTypedField<OuterType>::ShaderBufferFieldDecorations;
    using ShaderBufferTypedField<OuterType>::fieldDecorations;
    using ShaderBufferTypedField<OuterType>::paramInfo;
    using ShaderBufferTypedField<OuterType>::size;
    using ShaderBufferTypedField<OuterType>::stride;
    using ArrayType = IndexableType<MemberType>;

    using FieldPtr = ClassMemberField<false, OuterType, MemberType>;
    FieldPtr memberPtr;

    ShaderBufferMemberField(const String& pName, const FieldPtr& fieldPtr)
        : ShaderBufferTypedField<OuterType>(pName)
        , memberPtr(fieldPtr)
    {
        fieldDecorations |= std::is_array_v<MemberType> ? ShaderBufferFieldDecorations::IsArray : 0
            | std::is_pointer_v<MemberType> ? (ShaderBufferFieldDecorations::IsArray | ShaderBufferFieldDecorations::IsPointer) : 0;

        size = ConditionalValue_v<uint32, uint32, std::is_pointer<MemberType>, 0u, sizeof(MemberType)>;
        stride = sizeof(ArrayType);
    }

    constexpr static uint32 totalArrayElements()
    {
        return sizeof(MemberType)/sizeof(ArrayType);
    }

    template <typename DataType>
    constexpr static std::enable_if_t<IsIndexable<DataType>::value, IndexableType<DataType>>* memberDataPtr(DataType& data)
    {
        return &data[0];
    }

    template <typename DataType>
    constexpr static std::enable_if_t<std::negation_v<IsIndexable<DataType>>, DataType>* memberDataPtr(DataType& data)
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
        ArrayView<ArrayType> const*newValuesViewPtr = std::any_cast<ArrayView<ArrayType>>(&newValuesPtr);

        if (ShaderBufferTypedField<OuterType>::isIndexAccessible() && newValuesViewPtr != nullptr && newValuesViewPtr->data() != nullptr)
        {
            const ArrayType* newValues = newValuesViewPtr->data();
            if constexpr (std::is_pointer_v<MemberType>)
            {
                if (memberPtr.get(castOuterPtr) != nullptr)
                {
                    ArrayType* toValues = memberDataPtr(memberPtr.get(castOuterPtr));
                    memcpy(toValues, newValues, sizeof(ArrayType) * newValuesViewPtr->size());
                    return true;
                }
            }
            else
            {
                ArrayType* toValues = memberDataPtr(memberPtr.get(castOuterPtr));
                memcpy(toValues, newValues, Math::min(sizeof(MemberType), sizeof(ArrayType) * newValuesViewPtr->size()));
                return true;
            }
        }
        return false;
    }

    bool setFieldDataArray(void* outerPtr, const std::any& newValue, uint32 arrayIndex) const override
    {
        OuterType* castOuterPtr = reinterpret_cast<OuterType*>(outerPtr);
        const ArrayType* newValuePtr = std::any_cast<ArrayType>(&newValue);

        bool bCanSet = false;
        if constexpr (IsIndexable<MemberType>::value)
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
            ArrayType* toValues = memberDataPtr(memberPtr.get(castOuterPtr));
            (toValues)[arrayIndex] = *newValuePtr;
            return true;
        }
        return false;
    }

    void* fieldData(void* outerPtr, uint32* typeSize, uint32* elementSize) const override
    {
        if (typeSize)
        {
            (*typeSize) = sizeof(MemberType);
        }
        if (elementSize)
        {
            (*elementSize) = sizeof(ArrayType);
        }
        return memberDataPtr(memberPtr.get(reinterpret_cast<OuterType*>(outerPtr)));
    }
    
    void* fieldPtr(void* outerPtr) const override
    {
        return &memberPtr.get(reinterpret_cast<OuterType*>(outerPtr));
    }

    const void* fieldData(const OuterType* outerPtr, uint32* typeSize, uint32* elementSize) const override
    {
        if (typeSize)
        {
            (*typeSize) = sizeof(MemberType);
        }
        if (elementSize)
        {
            (*elementSize) = sizeof(ArrayType);
        }
        return memberDataPtr(memberPtr.get(outerPtr));
    }
};

template<typename OuterType, typename MemberType>
struct ShaderBufferStructField : public ShaderBufferMemberField<OuterType, MemberType>
{
    using ShaderBufferMemberField<OuterType, MemberType>::ShaderBufferFieldDecorations;
    using ShaderBufferMemberField<OuterType, MemberType>::fieldDecorations;
    using ShaderBufferMemberField<OuterType, MemberType>::paramInfo;
    using typename ShaderBufferMemberField<OuterType, MemberType>::FieldPtr;

    ShaderBufferStructField(const String& pName, const FieldPtr& fieldPtr, ShaderBufferParamInfo* pInfo)
        : ShaderBufferMemberField<OuterType, MemberType>(pName, fieldPtr)
    {
        paramInfo = pInfo;
        fieldDecorations |= ShaderBufferFieldDecorations::IsStruct;
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

    virtual uint32 paramStride() const = 0;
    virtual uint32 paramNativeStride() const = 0;
    virtual void setStride(uint32 newStride) = 0;

private:
    class IteratorBase
    {
    protected:
        const FieldNodeType* node;
    public:
        using FieldType = decltype(std::declval<FieldNodeType>().field);
        static_assert(std::is_pointer_v<FieldType>, "Field type must be pointer type");

        /* Iterator traits skipped difference_type as it does not makes sense */
        using value_type = FieldType;
        using reference = FieldType;
        using pointer = FieldType;
        using iterator_category = std::forward_iterator_tag;

        // End constructor, We assume it null rather than iterating through until end
        IteratorBase() : node(nullptr) {}
        IteratorBase(const ShaderParamInfo* paramInfo)
            : node(&paramInfo->startNode)
        {}
        IteratorBase(const IteratorBase& itr)
            : node(itr.node)
        {}
        IteratorBase(IteratorBase&& itr)
            : node(std::move(itr.node))
        {}
    };
public:
    class ConstIterator : public IteratorBase
    {
    private:
        using IteratorBase::node;
    public:
        using IteratorBase::FieldType;

        ConstIterator() = default;
        ConstIterator(const ShaderParamInfo* paramInfo)
            : IteratorBase(paramInfo)
        {}
        ConstIterator(const ConstIterator& itr)
            : IteratorBase(itr)
        {}
        ConstIterator(ConstIterator&& itr)
            : IteratorBase(std::forward(itr))
        {}

        const FieldType operator->() const
        {
            // Okay as FieldType is ptr
            return node ? node->field : nullptr;
        }

        const FieldType operator*() const
        {
            return node ? node->field : nullptr;
        }

        bool operator!=(const ConstIterator& other) const
        {
            const bool thisValid = node != nullptr && node->isValid();
            const bool otherValid = other.node != nullptr && other.node->isValid();
            return thisValid != otherValid || **this != *other;
        }

        ConstIterator& operator++()
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
        using IteratorBase::FieldType;

        Iterator() = default;
        Iterator(const ShaderParamInfo* paramInfo)
            : IteratorBase(paramInfo)
        {}
        Iterator(const Iterator& itr)
            : IteratorBase(itr)
        {}
        Iterator(Iterator&& itr)
            : IteratorBase(std::forward(itr))
        {}

        FieldType operator->() const
        {
            // Okay as FieldType is ptr
            return node ? node->field : nullptr;
        }

        FieldType operator*() const
        {
            return node ? node->field : nullptr;
        }

        bool operator!=(const Iterator& other) const
        {
            const bool thisValid = node != nullptr && node->isValid();
            const bool otherValid = other.node != nullptr && other.node->isValid();
            return thisValid != otherValid || **this != *other;
        }

        Iterator& operator++()
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

    ConstIterator begin() const
    {
        return ConstIterator(this);
    }

    ConstIterator end() const
    {
        return ConstIterator();
    }
    Iterator begin()
    {
        return Iterator(this);
    }

    Iterator end()
    {
        return Iterator();
    }
};

struct ENGINERENDERER_EXPORT ShaderVertexParamInfo : public ShaderParamInfo<ShaderVertexFieldNode>
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
    uint32 paramNativeStride() const { return sizeof(BufferDataType); } \
    void setStride(uint32 newStride) { stride = newStride; }

#define END_BUFFER_DEFINITION() \
}

#define ADD_BUFFER_TYPED_FIELD(FieldName) \
    ShaderBufferMemberField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field = { #FieldName, &BufferDataType::##FieldName }; \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field, &startNode };

// NOTE : Right now supporting : Buffer with any alignment
// but in case of inner struct only with proper alignment with respect to GPU(Alignment correction on copying to GPU is only done for first level of variables)  
#define ADD_BUFFER_STRUCT_FIELD(FieldName, FieldType) \
    FieldType##BufferParamInfo FieldName##ParamInfo; \
    ShaderBufferStructField<BufferDataType, decltype(BufferDataType::##FieldName##)> FieldName##Field = { #FieldName, &BufferDataType::##FieldName##, &##FieldName##ParamInfo }; \
    ShaderBufferFieldNode FieldName##Node = { &##FieldName##Field, &startNode };


#define BEGIN_VERTEX_DEFINITION(VertexType, InputFrequency) \
struct VertexType##VertexParamInfo final : public ShaderVertexParamInfo \
{ \
    typedef VertexType VertexDataType; \
    const EShaderInputFrequency::Type vertexInputFreq =  InputFrequency; \
    uint32 paramStride() const final { return sizeof(VertexDataType); } \
    uint32 paramNativeStride() const final { return sizeof(VertexDataType); } \
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