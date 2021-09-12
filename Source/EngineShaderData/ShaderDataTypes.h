#pragma once

#include "CommonShaderTypes.h"

struct ReflectTexelBufferShaderField;
struct ReflectTextureShaderField;

//////////////////////////////////////////////////////////////////////////
///// Textures, Sub pass inputs, samplers and Texel buffer related data
//////////////////////////////////////////////////////////////////////////

struct TexelComponentFormat
{
    EReflectBufferPrimitiveType type;// ReflectPrimitive_uint or ReflectPrimitive_int or ReflectPrimitive_float, if RelectPrimitive_invalid it means format doesn't matter or not known
    uint32_t componentCount;// R-1 ,RG-2, RGB-3, RGBA-4
    uint32_t componentSize[4] = {0,0,0,0};// Per component size
    bool bIsNormalized = false;
    bool bIsScaled = false;
};

// For texel samplerBuffer, imageBuffer and input attachments(array size will always be 1 for input attachments)
struct ReflectTexelBufferShaderField
{
    std::vector<ArrayDefinition> arraySize;// 1 in case of normal value and n in case of array
    TexelComponentFormat format;
};

// For texture, image, sampled image(sampler*)
struct ReflectTextureShaderField
{
    uint32_t imageViewType;
    std::vector<ArrayDefinition> arraySize;// 1 in case of normal value and n in case of array
    TexelComponentFormat format;
    bool bIsMultiSampled;
};

typedef uint32_t ReflectSubpassInput;// Sub pass input index
typedef std::vector<ArrayDefinition> ReflectSampler;

//////////////////////////////////////////////////////////////////////////
///// Descriptors set related data
//////////////////////////////////////////////////////////////////////////

namespace EDescriptorEntryState
{
    enum Flags
    {
        ReadOnly = 1,
        WriteOnly = 2
    };
}

// Descriptor reflection
template <typename DescriptorDataType>
struct DescriptorSetEntry
{
    uint8_t readWriteState;
    uint32_t binding;
    uint32_t stagesUsed;
    uint32_t type;
    DescriptorDataType data;
};

typedef NamedAttribute<DescriptorSetEntry<ReflectBufferShaderField>> DescEntryBuffer;
typedef NamedAttribute<DescriptorSetEntry<ReflectTexelBufferShaderField>> DescEntryTexelBuffer;
typedef NamedAttribute<DescriptorSetEntry<ReflectTextureShaderField>> DescEntryTexture;
typedef NamedAttribute<DescriptorSetEntry<ReflectSubpassInput>> DescEntrySubpassInput;
typedef NamedAttribute<DescriptorSetEntry<ReflectSampler>> DescEntrySampler;


struct ReflectDescriptorBody
{
    uint32_t set;
    std::vector<uint32_t> usedBindings;
    /*
    * 
    * Provides shader stages usage of all descriptors combined for this set.
    * 
    * NOTE : Useful in places where all descriptors of a set is used in every stage(none sparse usage like using *.inl to just include all, View or Vertex descriptors is one case)
    * As each descriptors will have same usage.
    * In case of sparse usage of descriptors will have different usage and this value might be inaccurate.
    */
    uint32_t combinedSetUsage;

    std::vector<DescEntryBuffer> uniforms;
    std::vector<DescEntryBuffer> buffers;
    std::vector<DescEntryTexelBuffer> samplerBuffers;
    std::vector<DescEntryTexelBuffer> imageBuffers;
    std::vector<DescEntryTexture> sampledTexAndArrays;// For sampler sampled images , and their array counter part
    std::vector<DescEntryTexture> textureAndArrays; // For non sampled images and their array counter part
    std::vector<DescEntrySubpassInput> subpassInputs;// Sub pass input attachments in set
    std::vector<DescEntryTexture> imagesAndImgArrays; // For storage images and their array counter part
    std::vector<DescEntrySampler> samplers;
};

//////////////////////////////////////////////////////////////////////////
///// Push constants related data
//////////////////////////////////////////////////////////////////////////

// Push constants reflection
struct PushConstantEntry
{
    uint32_t stagesUsed;
    ReflectBufferShaderField pushConstantField;
};
typedef NamedAttribute<PushConstantEntry> ReflectPushConstant;

//////////////////////////////////////////////////////////////////////////
///// Input, output related data
//////////////////////////////////////////////////////////////////////////

// For input and output(color attachments) types
struct InputOutputEntry
{
    uint32_t location;
    ReflectFieldType type;
};
typedef NamedAttribute<InputOutputEntry> ReflectInputOutput;

//////////////////////////////////////////////////////////////////////////
///// Specialization constants related data
//////////////////////////////////////////////////////////////////////////

// For specialization constant
struct SpecializationConstantDefaultValue
{
    union Value
    {
        bool boolVal;
        int i32Val;
        uint32_t u32Val;
        float f32Val;
        double f64Val;
    };
    SpecializationConstantDefaultValue()
    {
        memset(&defaultValue, 0, sizeof(defaultValue));
    }
    SpecializationConstantDefaultValue(bool value)
    {
        defaultValue.boolVal = value;
    }
    SpecializationConstantDefaultValue(int value)
    {
        defaultValue.i32Val = value;
    }
    SpecializationConstantDefaultValue(uint32_t value)
    {
        defaultValue.u32Val = value;
    }
    SpecializationConstantDefaultValue(float value)
    {
        defaultValue.f32Val = value;
    }
    SpecializationConstantDefaultValue(double value)
    {
        defaultValue.f64Val = value;
    }

    Value defaultValue;
};
// Only support scalar
struct SpecializationConstantEntry
{
    SpecializationConstantDefaultValue defaultValue;
    EReflectBufferPrimitiveType type;
    uint32_t constantId;
};
typedef NamedAttribute<SpecializationConstantEntry> ReflectSpecializationConstant;

namespace SpecializationConstUtility
{
    template<typename Type1, typename Type2>
    struct IsSameType
    {
        static constexpr bool value = false;
    };
    template<typename Type>
    struct IsSameType<Type, Type>
    {
        static constexpr bool value = true;
    };

    template<typename ValueType>
    constexpr EReflectBufferPrimitiveType toPrimitiveType()
    {
        if constexpr (IsSameType<bool, ValueType>::value)
        {
            return EReflectBufferPrimitiveType::ReflectPrimitive_bool;
        }
        else if constexpr (IsSameType<int, ValueType>::value)
        {
            return EReflectBufferPrimitiveType::ReflectPrimitive_int;
        }
        else if constexpr (IsSameType<uint32_t, ValueType>::value)
        {
            return EReflectBufferPrimitiveType::ReflectPrimitive_uint;
        }
        else if constexpr (IsSameType<float, ValueType>::value)
        {
            return EReflectBufferPrimitiveType::ReflectPrimitive_float;
        }
        else if constexpr (IsSameType<double, ValueType>::value)
        {
            return EReflectBufferPrimitiveType::ReflectPrimitive_double;
        }
        return EReflectBufferPrimitiveType::RelectPrimitive_invalid;
    }

    template<typename ValueType>
    SpecializationConstantEntry fromValue(ValueType value)
    {
        return { value, toPrimitiveType<ValueType>() };
    }

    template<typename ValueType>
    bool asValue(ValueType& value, const SpecializationConstantEntry& specializationConst)
    {
        if (toPrimitiveType<ValueType>() == specializationConst.type)
        {
            value = *reinterpret_cast<const ValueType*>(&specializationConst.defaultValue.defaultValue);
            return true;
        }
        return false;
    }
}
