#pragma once

#include <vector>
#include <vulkan_core.h>

struct ReflectBufferShaderField;
struct ReflectTexelBufferShaderField;
struct ReflectTextureShaderField;

//////////////////////////////////////////////////////////////////////////
///// Common data types
//////////////////////////////////////////////////////////////////////////

struct ArrayDefinition
{
    uint32_t dimension;// Will have specialization constant index if is specialization const is true
    bool isSpecializationConst;
};

template <typename AttributeType>
struct NamedAttribute
{
    std::string attributeName;
    AttributeType data;
};

template <typename StructField>
struct StructInnerFields
{
    uint32_t offset;
    uint32_t stride;// Individual primitive/inner struct stride
    uint32_t totalSize;// This is size of entire array in array field else will be equal to stride
    std::vector<ArrayDefinition> arraySize;// 1 in case of normal value and n in case of array
    StructField data;
};

// Primitive and hierarchy data types
enum EReflectBufferPrimitiveType
{
    RelectPrimitive_invalid = 0,
    ReflectPrimitive_bool = 1,
    ReflectPrimitive_int = 2,
    ReflectPrimitive_uint = 3,
    ReflectPrimitive_float = 4,
    ReflectPrimitive_double = 5,
};

struct ReflectFieldType
{
    EReflectBufferPrimitiveType primitive;
    uint32_t vecSize;
    uint32_t colSize;
};

//////////////////////////////////////////////////////////////////////////
///// Uniform and Storage buffers related data
//////////////////////////////////////////////////////////////////////////

// For both uniform and storage buffer as well as to push constant
// Single variable in a buffer
struct BufferEntry
{
    ReflectFieldType type;
};
typedef NamedAttribute<StructInnerFields<BufferEntry>> ReflectBufferEntry;
typedef NamedAttribute<StructInnerFields<ReflectBufferShaderField>> ReflectBufferStructEntry;

// For uniform, storage buffer and push constant
// Currently no AoS only SoA supported
struct ReflectBufferShaderField
{
    uint32_t stride = 0;// struct stride
    std::vector<ReflectBufferEntry> bufferFields;
    std::vector<ReflectBufferStructEntry> bufferStructFields;
};

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
    VkImageViewType imageViewType;
    std::vector<ArrayDefinition> arraySize;// 1 in case of normal value and n in case of array
    TexelComponentFormat format;
    bool bIsMultiSampled;
};

typedef uint32_t ReflectSubpassInput;// Sub pass input index
typedef std::vector<ArrayDefinition> ReflectSampler;

//////////////////////////////////////////////////////////////////////////
///// Descriptors set related data
//////////////////////////////////////////////////////////////////////////

// Descriptor reflection
template <typename DescriptorDataType>
struct DescriptorSetEntry
{
    uint32_t binding;
    VkPipelineStageFlags stagesUsed;
    VkDescriptorType type;
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
    VkPipelineStageFlags stagesUsed;
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

    Value defaltValue;
};
// Only support scalar
struct SpecializationConstantEntry
{
    SpecializationConstantDefaultValue defaultValue;
    EReflectBufferPrimitiveType type;
    uint32_t constantId;
};
typedef NamedAttribute<SpecializationConstantEntry> ReflectSpecializationConstant;