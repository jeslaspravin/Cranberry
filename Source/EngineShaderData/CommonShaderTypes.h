#pragma once


struct ReflectBufferShaderField;


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
