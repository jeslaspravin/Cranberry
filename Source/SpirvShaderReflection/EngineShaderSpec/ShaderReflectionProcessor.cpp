#include "ShaderReflectionProcessor.h"
#include "ShaderArchive.h"
#include "../SpirV/spirv_glsl.hpp"
#include "../SpirV/spirv_cross.hpp"
#include "../Utilities/CommonFunctions.h"

#include <iostream>
#include <assert.h>
#include <vulkan_core.h>
#include <set>
#include <algorithm>



ShaderReflectionProcessor::ShaderReflectionProcessor(std::string shaderFilePath)
    : shaderPath(shaderFilePath)
    , compiledData(nullptr)
{
    shaderFileName = shaderPath.substr(shaderPath.find_last_of("/\\") + 1);
    std::vector<unsigned char> data;
    if (CommonFunctions::readFromFile(shaderPath,data))
    {
        printf("Loaded shader file ----> %s\n", shaderPath.c_str());

        assert(data.size() % 4 == 0);
        shaderCode.resize(data.size() / 4);
        memcpy(shaderCode.data(), data.data(), data.size());
    }
    else
    {
        printf("Cannot open file %s\n", shaderPath.c_str());
        return;
    }

    compiledData = new SPIRV_CROSS_NAMESPACE::CompilerGLSL(shaderCode);
}

ShaderReflectionProcessor::ShaderReflectionProcessor(const std::vector<uint32_t>& code, const ShaderCodeView& view)
    : codeView(view)
{
    shaderCode.resize(codeView.size);
    memcpy(shaderCode.data(), &code[codeView.startIdx], codeView.size * sizeof(decltype(code[0])));
    compiledData = new SPIRV_CROSS_NAMESPACE::CompilerGLSL(shaderCode);
}

ShaderReflectionProcessor::~ShaderReflectionProcessor()
{
    delete compiledData;
    compiledData = nullptr;
}

void ShaderReflectionProcessor::injectShaderCode(std::vector<uint32_t>& codeCollector) const
{
    codeCollector.insert(codeCollector.end(), shaderCode.cbegin(), shaderCode.cend());
}

void ShaderReflectionProcessor::setCodeView(uint32_t startIndex, uint32_t size)
{
    codeView.startIdx = startIndex;
    codeView.size = size;
}

ShaderStageDescription ShaderReflectionProcessor::getStageDesc() const
{
    SPIRV_CROSS_NAMESPACE::SmallVector<SPIRV_CROSS_NAMESPACE::EntryPoint> entryPoints = compiledData->get_entry_points_and_stages();
    // Since we support only one entry per shader file
    assert(entryPoints.size() == 1);

    ShaderStageDescription returnVal;
    returnVal.entryPoint = entryPoints[0].name;
    returnVal.codeView = codeView;
    returnVal.stage = engineStage(entryPoints[0].execution_model);
    returnVal.pipelineBindPoint = pipelineBindPoint(entryPoints[0].execution_model);

    return returnVal;
}

// common utility functions

uint32_t ShaderReflectionProcessor::engineStage(spv::ExecutionModel spirvStage)
{
    switch (spirvStage)
    {
    case spv::ExecutionModelVertex:
        return VERTEX_STAGE;
    case spv::ExecutionModelTessellationControl:
        return TESS_CONTROL_STAGE;
    case spv::ExecutionModelTessellationEvaluation:
        return TESS_EVAL_STAGE;
    case spv::ExecutionModelGeometry:
        return GEOMETRY_STAGE;
    case spv::ExecutionModelFragment:
        return FRAGMENT_STAGE;
    case spv::ExecutionModelGLCompute:
        return COMPUTE_STAGE;
    default:
        assert(!"Unsupported shader stage");
        printf("ERROR: [%s]  Shader stage %d of spv::ExecutionModel is not supported\n", __func__, spirvStage);
        break;
    }
    return 0;
}

uint32_t ShaderReflectionProcessor::pipelineBindPoint(spv::ExecutionModel spirvStage)
{
    switch (spirvStage)
    {
    case spv::ExecutionModelVertex:
    case spv::ExecutionModelTessellationControl:
    case spv::ExecutionModelTessellationEvaluation:
    case spv::ExecutionModelGeometry:
    case spv::ExecutionModelFragment:
        return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
    case spv::ExecutionModelGLCompute:
        return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
    default:
        assert(!"Unsupported shader stage");
        printf("ERROR: [%s] Shader stage %d of spv::ExecutionModel is not supported\n", __func__, spirvStage);
        break;
    }
    return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
}


uint32_t ShaderReflectionProcessor::pipelineStageFlag(spv::ExecutionModel spirvStage)
{
    switch (spirvStage)
    {
    case spv::ExecutionModelVertex:
        return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    case spv::ExecutionModelTessellationControl:
        return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
    case spv::ExecutionModelTessellationEvaluation:
        return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    case spv::ExecutionModelGeometry:
        return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    case spv::ExecutionModelFragment:
        return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case spv::ExecutionModelGLCompute:
        return VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    default:
        assert(!"Unsupported pipeline stage");
        printf("ERROR: [%s] Shader stage %d of spv::ExecutionModel is not supported\n", __func__, spirvStage);
        break;
    }
    return 0;
}

uint32_t ShaderReflectionProcessor::shaderStageFlag(spv::ExecutionModel spirvStage)
{
    switch (spirvStage)
    {
    case spv::ExecutionModelVertex:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    case spv::ExecutionModelTessellationControl:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case spv::ExecutionModelTessellationEvaluation:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case spv::ExecutionModelGeometry:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
    case spv::ExecutionModelFragment:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    case spv::ExecutionModelGLCompute:
        return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
    default:
        assert(!"Unsupported shader stage");
        printf("ERROR: [%s] Shader stage %d of spv::ExecutionModel is not supported\n", __func__, spirvStage);
        break;
    }
    return 0;
}

uint32_t ShaderReflectionProcessor::imageViewType(spv::Dim spirvDim, bool bIsArray)
{
    switch (spirvDim)
    {
    case spv::Dim1D:
        return bIsArray? VkImageViewType::VK_IMAGE_VIEW_TYPE_1D_ARRAY : VkImageViewType::VK_IMAGE_VIEW_TYPE_1D;
    case spv::Dim2D:
        return bIsArray ? VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY : VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    case spv::Dim3D:
        return VkImageViewType::VK_IMAGE_VIEW_TYPE_3D;
    case spv::DimCube:
        return bIsArray ? VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
    case spv::DimBuffer:
    case spv::DimRect:
    case spv::DimSubpassData:
    case spv::DimMax:
    default:
        assert(!"Unsupported view type");
        printf("ERROR: [%s] spv::Dim %d supported\n", __func__, spirvDim);
        break;
    }
    return VkImageViewType::VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

TexelComponentFormat ShaderReflectionProcessor::texelFormat(spv::ImageFormat format)
{
    switch (format)
    {
    case spv::ImageFormatR16f:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_float, 1, {16,0,0,0}, false , false };
    case spv::ImageFormatRg16f:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_float, 2, {16,16,0,0}, false , false };
    case spv::ImageFormatRgba16f:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_float, 4, {16,16,16,16}, false, false };
    case spv::ImageFormatR32f:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_float, 1, {32,0,0,0}, false, false };
    case spv::ImageFormatRg32f:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_float, 2, {32,32,0,0}, false , false };
    case spv::ImageFormatRgba32f:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_float, 4, {32,32,32,32}, false, false };
    case spv::ImageFormatR11fG11fB10f:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_float, 3, {11,11,10,0}, false , false };
    case spv::ImageFormatR8:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 1, {8,0,0,0}, false , true };
    case spv::ImageFormatR8Snorm:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 1, {8,0,0,0}, true , false };
    case spv::ImageFormatRg8:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 2, {8,8,0,0}, false , true };
    case spv::ImageFormatRg8Snorm:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 2, {8,8,0,0}, true , false };
    case spv::ImageFormatRgba8:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {8,8,8,8}, false, true };
    case spv::ImageFormatRgba8Snorm:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {8,8,8,8}, true , false };
    case spv::ImageFormatRgb10A2:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {10,10,10,2}, false, true };
    case spv::ImageFormatR16:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 1, {16,0,0,0}, false , true };
    case spv::ImageFormatR16Snorm:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 1, {16,0,0,0}, true , false };
    case spv::ImageFormatRg16:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 2, {16,16,0,0}, false , true };
    case spv::ImageFormatRg16Snorm:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 2, {16,16,0,0}, true , false };
    case spv::ImageFormatRgba16:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {16,16,16,16}, false , true };
    case spv::ImageFormatRgba16Snorm:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {16,16,16,16}, true , false };
    case spv::ImageFormatR8i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 1, {8,0,0,0}, false, false };
    case spv::ImageFormatR16i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 1, {16,0,0,0}, false, false };
    case spv::ImageFormatR32i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 1, {32,0,0,0}, false, false };
    case spv::ImageFormatRg8i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 2, {8,8,0,0}, false, false };
    case spv::ImageFormatRg16i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 2, {16,16,0,0}, false, false };
    case spv::ImageFormatRg32i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 2, {32,32,0,0}, false, false };
    case spv::ImageFormatRgba8i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {8,8,8,8}, false, false };
    case spv::ImageFormatRgba16i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {16,16,16,16}, false , false };
    case spv::ImageFormatRgba32i:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_int, 4, {32,32,32,32}, false, false };
    case spv::ImageFormatR8ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 1, {8,0,0,0}, false, false };
    case spv::ImageFormatR16ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 1, {16,0,0,0}, false, false };
    case spv::ImageFormatR32ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 1, {32,0,0,0}, false, false };
    case spv::ImageFormatRg8ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 2, {8,8,0,0}, false, false };
    case spv::ImageFormatRg16ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 2, {16,16,0,0}, false, false };
    case spv::ImageFormatRg32ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 2, {32,32,0,0}, false, false };
    case spv::ImageFormatRgba8ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 4, {8,8,8,8}, false, false };
    case spv::ImageFormatRgb10a2ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 4, {10,10,10,2}, false, false };
    case spv::ImageFormatRgba16ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 4, {16,16,16,16}, false, false };
    case spv::ImageFormatRgba32ui:
        return { EReflectBufferPrimitiveType::ReflectPrimitive_uint, 4, {32,32,32,32}, false, false };
    case spv::ImageFormatUnknown:
        break;
    case spv::ImageFormatMax:
    default:
        assert(!"Unsupported texel format");
        printf("ERROR: [%s] spv::ImageFormat %d supported\n", __func__, format);
        break;
    }
    return { EReflectBufferPrimitiveType::RelectPrimitive_invalid, 4, {0,0,0,0}, false, false };
}

EReflectBufferPrimitiveType getReflectPrimitiveType(const SPIRV_CROSS_NAMESPACE::SPIRType::BaseType& type)
{
    switch (type)
    {
    case SPIRV_CROSS_NAMESPACE::SPIRType::Boolean:
        return EReflectBufferPrimitiveType::ReflectPrimitive_bool;
    case SPIRV_CROSS_NAMESPACE::SPIRType::Int:
        return EReflectBufferPrimitiveType::ReflectPrimitive_int;
    case SPIRV_CROSS_NAMESPACE::SPIRType::UInt:
        return EReflectBufferPrimitiveType::ReflectPrimitive_uint;
    case SPIRV_CROSS_NAMESPACE::SPIRType::Float:
        return EReflectBufferPrimitiveType::ReflectPrimitive_float;
    case SPIRV_CROSS_NAMESPACE::SPIRType::Double:
        return EReflectBufferPrimitiveType::ReflectPrimitive_double;
    case SPIRV_CROSS_NAMESPACE::SPIRType::Unknown:
        return EReflectBufferPrimitiveType::RelectPrimitive_invalid;
    case SPIRV_CROSS_NAMESPACE::SPIRType::SByte:
    case SPIRV_CROSS_NAMESPACE::SPIRType::UByte:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Short:
    case SPIRV_CROSS_NAMESPACE::SPIRType::UShort:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Int64:
    case SPIRV_CROSS_NAMESPACE::SPIRType::UInt64:
    case SPIRV_CROSS_NAMESPACE::SPIRType::AtomicCounter:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Half:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Struct:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Image:
    case SPIRV_CROSS_NAMESPACE::SPIRType::SampledImage:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Sampler:
    case SPIRV_CROSS_NAMESPACE::SPIRType::AccelerationStructure:
    case SPIRV_CROSS_NAMESPACE::SPIRType::RayQuery:
    case SPIRV_CROSS_NAMESPACE::SPIRType::ControlPointArray:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Char:
    case SPIRV_CROSS_NAMESPACE::SPIRType::Void:
    default:
        break;
    }
    return EReflectBufferPrimitiveType::RelectPrimitive_invalid;
}

void setSpecializationConstDefault(SpecializationConstantDefaultValue& value, const SPIRV_CROSS_NAMESPACE::SPIRConstant& constantRef
    , const SPIRV_CROSS_NAMESPACE::SPIRType& typeRef)
{
    switch (typeRef.basetype)
    {
    case SPIRV_CROSS_NAMESPACE::SPIRType::Boolean:
        value.defaultValue.boolVal = constantRef.scalar();
        break;
    case SPIRV_CROSS_NAMESPACE::SPIRType::Int:
        value.defaultValue.i32Val = constantRef.scalar_i32();
        break;
    case SPIRV_CROSS_NAMESPACE::SPIRType::UInt:
        value.defaultValue.u32Val = constantRef.scalar();
        break;
    case SPIRV_CROSS_NAMESPACE::SPIRType::Float:
        value.defaultValue.f32Val = constantRef.scalar_f32();
    case SPIRV_CROSS_NAMESPACE::SPIRType::Double:
        value.defaultValue.f64Val = constantRef.scalar_f64();
        break;
    default:
        value.defaultValue.f64Val = 0;
        break;
    }
}

bool validateSpecializationConst(const SPIRV_CROSS_NAMESPACE::SPIRConstant& constantRef, const SPIRV_CROSS_NAMESPACE::SPIRType& typeRef)
{
    if (!typeRef.array.empty())
    {
        printf("ERROR: [%s] unsupported specialization constant, cannot use array type in specialization constant", __func__);
        return false;
    }

    if (getReflectPrimitiveType(typeRef.basetype) == RelectPrimitive_invalid)
    {
        printf("ERROR: [%s] unsupported specialization constant primitive type", __func__);
        return false;
    }
    return true;
}

ReflectFieldType getReflectedType(const SPIRV_CROSS_NAMESPACE::SPIRType& baseType)
{
    ReflectFieldType fieldType;
    fieldType.primitive = getReflectPrimitiveType(baseType.basetype);
    fieldType.vecSize = baseType.vecsize;
    fieldType.colSize = baseType.columns;
    return fieldType;
}

void fillBufferFieldArrayInfo(std::vector<ArrayDefinition>& arrayDefs, const SPIRV_CROSS_NAMESPACE::SPIRType& type
    , const std::map<uint32_t, uint32_t>& specConstMap, const uint32_t& stageIdx)
{
    if (type.array.empty())
    {
        arrayDefs.clear();
        ArrayDefinition def{ 1 };
        arrayDefs.push_back(def);
    }
    else
    {
        arrayDefs.resize(type.array.size());
        for (int i = 0; i < type.array.size(); ++i)
        {
            ArrayDefinition& def = arrayDefs[type.array.size() - 1 - i];
            if (type.array_size_literal[i])
            {
                def.dimension = type.array[i];
                def.isSpecializationConst = false;
            }
            else
            {
                def.isSpecializationConst = true;
                auto itr = specConstMap.find(type.array[i]);
                if (itr == specConstMap.cend())
                {
                    assert(!"Failed to find specialization const ID in map");
                    printf("ERROR: Failed to find specialization const ID in map for ID %d", type.array[i]);
                    continue;
                }
                def.dimension = itr->second;
                def.stageIdx = stageIdx;
            }
        }
    }
}

void fillBufferFields(ReflectBufferShaderField& shaderBufferField, const SPIRV_CROSS_NAMESPACE::SPIRType& structType
    , const SPIRV_CROSS_NAMESPACE::Compiler* compiledData, const std::map<uint32_t, uint32_t>& specConstMap, const uint32_t& stageIdx)
{
    uint32_t index = 0;
    // getting max of all comparison as value with max stride is last of buffer struct of all stages
    uint32_t newStride = uint32_t(compiledData->get_declared_struct_size(structType));
    shaderBufferField.stride = shaderBufferField.stride > newStride? shaderBufferField.stride : newStride;
    for (const SPIRV_CROSS_NAMESPACE::TypeID& memberTypeID : structType.member_types)
    {
        const SPIRV_CROSS_NAMESPACE::SPIRType& memberType = compiledData->get_type(memberTypeID);
        if (memberType.basetype == SPIRV_CROSS_NAMESPACE::SPIRType::BaseType::Struct)
        {
            ReflectBufferStructEntry innerStruct;
            innerStruct.attributeName = compiledData->get_member_name(structType.self, index);
            innerStruct.data.totalSize = uint32_t(compiledData->get_declared_struct_member_size(structType, index));
            if (memberType.array.empty())
            {
                innerStruct.data.stride = innerStruct.data.data.stride = uint32_t(compiledData->get_declared_struct_member_size(structType, index));
            }
            else
            {
                innerStruct.data.stride = innerStruct.data.data.stride = uint32_t(compiledData->type_struct_member_array_stride(structType, index));
            }
            innerStruct.data.offset = uint32_t(compiledData->type_struct_member_offset(structType, index));
            fillBufferFieldArrayInfo(innerStruct.data.arraySize, memberType, specConstMap, stageIdx);
            fillBufferFields(innerStruct.data.data, memberType, compiledData, specConstMap, stageIdx);

            shaderBufferField.bufferStructFields.push_back(innerStruct);
        }
        else
        {
            ReflectBufferEntry memberField;
            memberField.attributeName = compiledData->get_member_name(structType.self, index).c_str();
            memberField.data.totalSize = memberField.data.stride = uint32_t(compiledData->get_declared_struct_member_size(structType, index));

            if (memberType.columns > 1)
            {
                memberField.data.stride = compiledData->type_struct_member_matrix_stride(structType, index);
            }
            else if (!memberType.array.empty())
            {
                memberField.data.stride = compiledData->type_struct_member_array_stride(structType, index);
            }
            memberField.data.offset = compiledData->type_struct_member_offset(structType, index);
            memberField.data.data.type = getReflectedType(memberType);
            fillBufferFieldArrayInfo(memberField.data.arraySize, memberType, specConstMap, stageIdx);

            shaderBufferField.bufferFields.push_back(memberField);
        }
        index++;
    }
}

template<typename Type>
bool offsetSortFunc(const StructInnerFields<Type>& lhs, const StructInnerFields<Type>& rhs)
{
    return rhs.offset > lhs.offset;
}

void squashDuplicates(ReflectBufferShaderField& shaderBufferField)
{
    // Squashing BufferField entries
    {
        std::vector<ReflectBufferEntry> squashedBufferFields;
        squashedBufferFields.reserve(shaderBufferField.bufferFields.size());

        sort(shaderBufferField.bufferFields.begin(), shaderBufferField.bufferFields.end()
            , [](const ReflectBufferEntry& rhs, const ReflectBufferEntry& lhs)
            {
                return offsetSortFunc(rhs.data, lhs.data);
            });

        for (uint32_t i = 0; i < shaderBufferField.bufferFields.size();)
        {
            uint32_t j = i + 1;
            for (; j < shaderBufferField.bufferFields.size() && shaderBufferField.bufferFields[i].data.offset == shaderBufferField.bufferFields[j].data.offset; ++j);

            squashedBufferFields.push_back(shaderBufferField.bufferFields[i]);
            i = j;
        }

        squashedBufferFields.shrink_to_fit();
        if (shaderBufferField.bufferFields.size() != squashedBufferFields.size())
        {
            shaderBufferField.bufferFields = squashedBufferFields;
        }
    }
    // Squashing Buffer Struct entries
    {
        std::vector<ReflectBufferStructEntry> squashedStructFields;
        squashedStructFields.reserve(shaderBufferField.bufferStructFields.size());

        sort(shaderBufferField.bufferStructFields.begin(), shaderBufferField.bufferStructFields.end()
            , [](const ReflectBufferStructEntry& rhs, const ReflectBufferStructEntry& lhs)
            {
                return offsetSortFunc(rhs.data, lhs.data);
            });

        for (uint32_t i = 0; i < shaderBufferField.bufferStructFields.size();)
        {
            ReflectBufferShaderField& uniqueStructField = shaderBufferField.bufferStructFields[i].data.data;

            uint32_t j = i + 1;
            for (; j < shaderBufferField.bufferStructFields.size() && shaderBufferField.bufferStructFields[i].data.offset
                == shaderBufferField.bufferStructFields[j].data.offset; ++j)
            {
                ReflectBufferShaderField& duplicateStructField = shaderBufferField.bufferStructFields[j].data.data;
                uniqueStructField.bufferFields.insert(uniqueStructField.bufferFields.end(), duplicateStructField.bufferFields.cbegin()
                    , duplicateStructField.bufferFields.cend());
                uniqueStructField.bufferStructFields.insert(uniqueStructField.bufferStructFields.end()
                    , duplicateStructField.bufferStructFields.cbegin(), duplicateStructField.bufferStructFields.cend());

                // If some duplicate has higher stride? use that value, happens when inner struct is also used selectively between shaders.
                if (shaderBufferField.bufferStructFields[j].data.stride > shaderBufferField.bufferStructFields[i].data.stride)
                {
                    shaderBufferField.bufferStructFields[i].data.stride = shaderBufferField.bufferStructFields[i].data.data.stride = shaderBufferField.bufferStructFields[j].data.stride;
                    shaderBufferField.bufferStructFields[i].data.totalSize = shaderBufferField.bufferStructFields[j].data.totalSize;
                }                
            }
            // Recursively squash
            squashDuplicates(uniqueStructField);
            squashedStructFields.push_back(shaderBufferField.bufferStructFields[i]);
            i = j;
        }

        squashedStructFields.shrink_to_fit();
        if (shaderBufferField.bufferStructFields.size() != squashedStructFields.size())
        {
            shaderBufferField.bufferStructFields = squashedStructFields;
        }
    }
}

void fillSampledImageFormats(TexelComponentFormat& format, const SPIRV_CROSS_NAMESPACE::SPIRType& baseType)
{
    format = ShaderReflectionProcessor::texelFormat(baseType.image.format);
    format.componentCount = 4;// Always four in sampled image
}

// Sort descriptors entry based on binding
template<typename Type>
bool sortDescriptors(NamedAttribute<DescriptorSetEntry<Type>>& lhsEntry, NamedAttribute<DescriptorSetEntry<Type>>& rhsEntry)
{
    return lhsEntry.data.binding > rhsEntry.data.binding;
}

// Generic descriptors set duplicates removing
template<typename Type>
void squashDuplicates(std::vector<NamedAttribute<DescriptorSetEntry<Type>>>& descriptorsCollection)
{
    std::vector<NamedAttribute<DescriptorSetEntry<Type>>> squashedDescriptors;
    squashedDescriptors.reserve(descriptorsCollection.size());
    for (uint32_t i = 0; i < descriptorsCollection.size();)
    {
        NamedAttribute<DescriptorSetEntry<Type>>& uniqueDescriptor = descriptorsCollection[i];
        uint32_t j = i + 1;
        for (; j < descriptorsCollection.size() && uniqueDescriptor.data.binding == descriptorsCollection[j].data.binding; ++j)
        {
            NamedAttribute<DescriptorSetEntry<Type>>& duplicateDescriptor = descriptorsCollection[j];
            uniqueDescriptor.data.stagesUsed |= duplicateDescriptor.data.stagesUsed;
        }

        squashedDescriptors.push_back(uniqueDescriptor);
        i = j;
    }

    squashedDescriptors.shrink_to_fit();
    if (descriptorsCollection.size() != squashedDescriptors.size())
    {
        descriptorsCollection = squashedDescriptors;
    }
}

template<>
void squashDuplicates<ReflectBufferShaderField>(std::vector<NamedAttribute<DescriptorSetEntry<ReflectBufferShaderField>>>& descriptorsCollection)
{
    std::vector<DescEntryBuffer> squashedBufferFields;
    squashedBufferFields.reserve(descriptorsCollection.size());
    for (uint32_t i = 0; i < descriptorsCollection.size();)
    {
        DescEntryBuffer& uniqueBuffer = descriptorsCollection[i];
        uint32_t j = i + 1;
        for (; j < descriptorsCollection.size() && uniqueBuffer.data.binding == descriptorsCollection[j].data.binding; ++j)
        {
            DescEntryBuffer& duplicateBuffer = descriptorsCollection[j];

            uniqueBuffer.data.stagesUsed |= duplicateBuffer.data.stagesUsed;
            uniqueBuffer.data.data.bufferFields.insert(uniqueBuffer.data.data.bufferFields.end(), duplicateBuffer.data.data.bufferFields.cbegin()
                , duplicateBuffer.data.data.bufferFields.cend());
            uniqueBuffer.data.data.bufferStructFields.insert(uniqueBuffer.data.data.bufferStructFields.end()
                , duplicateBuffer.data.data.bufferStructFields.cbegin(), duplicateBuffer.data.data.bufferStructFields.cend());

            // If some duplicate has higher stride? use that value, happens when struct is used selectively between shaders.
            if (duplicateBuffer.data.data.stride > uniqueBuffer.data.data.stride)
            {
                uniqueBuffer.data.data.stride = duplicateBuffer.data.data.stride;
            }
        }

        squashDuplicates(uniqueBuffer.data.data);
        squashedBufferFields.push_back(uniqueBuffer);
        i = j;
    }

    squashedBufferFields.shrink_to_fit();
    if (descriptorsCollection.size() != squashedBufferFields.size())
    {
        descriptorsCollection = squashedBufferFields;
    }
}

void squashDuplicates(ReflectDescriptorBody& descriptorsSet)
{
    sort(descriptorsSet.uniforms.begin(), descriptorsSet.uniforms.end(), &sortDescriptors<ReflectBufferShaderField>);
    sort(descriptorsSet.buffers.begin(), descriptorsSet.buffers.end(), &sortDescriptors<ReflectBufferShaderField>);
    sort(descriptorsSet.samplerBuffers.begin(), descriptorsSet.samplerBuffers.end(), &sortDescriptors<ReflectTexelBufferShaderField>);
    sort(descriptorsSet.imageBuffers.begin(), descriptorsSet.imageBuffers.end(), &sortDescriptors<ReflectTexelBufferShaderField>);
    sort(descriptorsSet.sampledTexAndArrays.begin(), descriptorsSet.sampledTexAndArrays.end(), &sortDescriptors<ReflectTextureShaderField>);
    sort(descriptorsSet.textureAndArrays.begin(), descriptorsSet.textureAndArrays.end(), &sortDescriptors<ReflectTextureShaderField>);
    sort(descriptorsSet.imagesAndImgArrays.begin(), descriptorsSet.imagesAndImgArrays.end(), &sortDescriptors<ReflectTextureShaderField>);
    sort(descriptorsSet.subpassInputs.begin(), descriptorsSet.subpassInputs.end(), &sortDescriptors<ReflectSubpassInput>);
    sort(descriptorsSet.samplers.begin(), descriptorsSet.samplers.end(), &sortDescriptors<ReflectSampler>);

    // Uniform buffers
    squashDuplicates(descriptorsSet.uniforms);
    squashDuplicates(descriptorsSet.buffers);
    squashDuplicates(descriptorsSet.samplerBuffers);
    squashDuplicates(descriptorsSet.imageBuffers);
    squashDuplicates(descriptorsSet.sampledTexAndArrays);
    squashDuplicates(descriptorsSet.textureAndArrays);
    squashDuplicates(descriptorsSet.imagesAndImgArrays);
    squashDuplicates(descriptorsSet.subpassInputs);
    squashDuplicates(descriptorsSet.samplers);
}

// Combines the descriptors usage of list of same typed descriptors
template<typename Type>
uint32_t combinedDescritorsUsage(const std::vector<NamedAttribute<DescriptorSetEntry<Type>>>& descriptorsCollection)
{
    uint32_t combinedUsage = 0;
    for (const NamedAttribute<DescriptorSetEntry<Type>>& descriptor : descriptorsCollection)
    {
        combinedUsage |= descriptor.data.stagesUsed;
    }
    return combinedUsage;
}

uint32_t combinedDescritorsUsage(const ReflectDescriptorBody& descriptorsSet)
{
    return combinedDescritorsUsage(descriptorsSet.uniforms)         |
        combinedDescritorsUsage(descriptorsSet.buffers)             |
        combinedDescritorsUsage(descriptorsSet.samplerBuffers)      |
        combinedDescritorsUsage(descriptorsSet.imageBuffers)        |
        combinedDescritorsUsage(descriptorsSet.sampledTexAndArrays) |
        combinedDescritorsUsage(descriptorsSet.textureAndArrays)    |
        combinedDescritorsUsage(descriptorsSet.imagesAndImgArrays)  |
        combinedDescritorsUsage(descriptorsSet.subpassInputs)       |
        combinedDescritorsUsage(descriptorsSet.samplers);
}

void printArrayDefs(const std::vector<ArrayDefinition>& arrayDefs, std::string indent)
{
    printf("%sArraySize : ",indent.c_str());
    for (const ArrayDefinition& def : arrayDefs)
    {
        printf("[%d : %s(%d)]", def.dimension, def.isSpecializationConst ? "true" : "false", def.stageIdx);
    }
    printf("\n");
}

void printReflectedType(const ReflectFieldType& fieldType,std::string indent)
{
    printf("%sPrimitive type : %d[%d][%d]\n", indent.c_str(), fieldType.primitive, fieldType.vecSize, fieldType.colSize);
}

void printFields(const ReflectBufferShaderField& shaderBufferField,std::string indent)
{
    const char* indentChar = indent.c_str();
    printf("%sStride : %d\n", indentChar, shaderBufferField.stride);

    for (uint32_t bufferIdx = 0, structIdx = 0; bufferIdx < shaderBufferField.bufferFields.size() || structIdx < shaderBufferField.bufferStructFields.size();)
    {
        // valid bufferIdx and either structIdx is invalid or bufferField[bufferIdx] offset is less than that at structIdx
        if (bufferIdx < shaderBufferField.bufferFields.size() && (structIdx >= shaderBufferField.bufferStructFields.size() 
            || shaderBufferField.bufferFields[bufferIdx].data.offset < shaderBufferField.bufferStructFields[structIdx].data.offset))
        {
            const ReflectBufferEntry& bufferField = shaderBufferField.bufferFields[bufferIdx];

            printf("%sField : %s\n%s\tStride : %d\n%s\tOffset : %d\n%s\tTotalSize : %d\n", indentChar
                , bufferField.attributeName.c_str(), indentChar, bufferField.data.stride
                , indentChar, bufferField.data.offset, indentChar, bufferField.data.totalSize);
            printReflectedType(bufferField.data.data.type, indent+"\t");
            printArrayDefs(bufferField.data.arraySize,indent + "\t");

            ++bufferIdx;
        }
        else // Definitely valid structIdx when entering this block
        {
            const ReflectBufferStructEntry& structField = shaderBufferField.bufferStructFields[structIdx];

            printf("%sStruct : %s\n%s\tStride : %d\n%s\tOffset : %d\n%s\tTotalSize : %d\n", indentChar
                , structField.attributeName.c_str(), indentChar, structField.data.stride
                , indentChar, structField.data.offset, indentChar, structField.data.totalSize);
            printArrayDefs(structField.data.arraySize, indent + "\t");
            printFields(structField.data.data, indent + '\t');

            ++structIdx;
        }
    }
}

template <typename Type>
void printDescriptorDesc(const NamedAttribute<DescriptorSetEntry<Type>>& descriptor,std::string indent)
{
    const char* indentChar = indent.c_str();
    printf("  Binding = %d\n%sName : %s\n%sDescriptor Type : %d\n%sPipeline stages used : %d\n"
        , descriptor.data.binding, indentChar, descriptor.attributeName.c_str(), indentChar
        , descriptor.data.type, indentChar, descriptor.data.stagesUsed);
}

void printTexelCompFormat(const TexelComponentFormat& componentFormat, std::string indent)
{
    const char* indentChar = indent.c_str();
    printf("%sComponent Type : %d\n", indentChar, componentFormat.type);
    static char compChars[] = { 'R','G','B','A' };
    printf("%sComponents : ", indentChar);
    for (uint32_t i = 0; i < componentFormat.componentCount; ++i)
    {
        printf("%c",compChars[i]);
    }
    printf("\n");

    if (componentFormat.type != EReflectBufferPrimitiveType::RelectPrimitive_invalid)
    {
        printf("%sComponent size(in bits) : ", indentChar);
        for (uint32_t i = 0; i < componentFormat.componentCount; ++i)
        {
            printf("[%d]", componentFormat.componentSize[i]);
        }
        printf("\n");
        if(componentFormat.type != EReflectBufferPrimitiveType::ReflectPrimitive_float)// Only in integers this matters
        {
            printf("\n%sIs Normalized : %s\n", indentChar, componentFormat.bIsNormalized ? "true" : "false");
            printf("\n%sIs Scaled : %s\n", indentChar, componentFormat.bIsNormalized ? "true" : "false");
        }
    }
}

void printDescriptorsSet(const ReflectDescriptorBody& descriptorsSet)
{
    printf("Descriptors Set = %d Combined stages usage = %d\n", descriptorsSet.set, descriptorsSet.combinedSetUsage);
    for (const uint32_t& binding : descriptorsSet.usedBindings)
    {
        auto finderFunc = [&binding](const auto& descriptor) {
            return descriptor.data.binding == binding;
        };

        {
            auto itr = std::find_if(descriptorsSet.uniforms.cbegin(), descriptorsSet.uniforms.cend(), finderFunc);
            if (itr != descriptorsSet.uniforms.cend())
            {
                printDescriptorDesc(*itr,"\t");
                printFields(itr->data.data,"\t");
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.buffers.cbegin(), descriptorsSet.buffers.cend(), finderFunc);
            if (itr != descriptorsSet.buffers.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printFields(itr->data.data, "\t");
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.samplerBuffers.cbegin(), descriptorsSet.samplerBuffers.cend(), finderFunc);
            if (itr != descriptorsSet.samplerBuffers.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printTexelCompFormat(itr->data.data.format, "\t");
                printArrayDefs(itr->data.data.arraySize, "\t");
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.imageBuffers.cbegin(), descriptorsSet.imageBuffers.cend(), finderFunc);
            if (itr != descriptorsSet.imageBuffers.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printTexelCompFormat(itr->data.data.format, "\t");
                printArrayDefs(itr->data.data.arraySize, "\t");
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.sampledTexAndArrays.cbegin(), descriptorsSet.sampledTexAndArrays.cend(), finderFunc);
            if (itr != descriptorsSet.sampledTexAndArrays.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printf("\tImage view type : %d\n\tIs multi sampled : %s\n", itr->data.data.imageViewType
                    , itr->data.data.bIsMultiSampled ? "true" : "false");
                printTexelCompFormat(itr->data.data.format,"\t");
                printArrayDefs(itr->data.data.arraySize, "\t");
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.textureAndArrays.cbegin(), descriptorsSet.textureAndArrays.cend(), finderFunc);
            if (itr != descriptorsSet.textureAndArrays.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printf("\tImage view type : %d\n\tIs multi sampled : %s\n", itr->data.data.imageViewType
                    , itr->data.data.bIsMultiSampled ? "true" : "false");
                printTexelCompFormat(itr->data.data.format, "\t");
                printArrayDefs(itr->data.data.arraySize, "\t");
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.imagesAndImgArrays.cbegin(), descriptorsSet.imagesAndImgArrays.cend(), finderFunc);
            if (itr != descriptorsSet.imagesAndImgArrays.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printf("\tImage view type : %d\n\tIs multi sampled : %s\n", itr->data.data.imageViewType
                    , itr->data.data.bIsMultiSampled ? "true" : "false");
                printTexelCompFormat(itr->data.data.format, "\t");
                printArrayDefs(itr->data.data.arraySize, "\t");
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.subpassInputs.cbegin(), descriptorsSet.subpassInputs.cend(), finderFunc);
            if (itr != descriptorsSet.subpassInputs.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printf("\tSubpass Input attachment index : %d\n", itr->data.data);
                continue;
            }
        }
        {
            auto itr = std::find_if(descriptorsSet.samplers.cbegin(), descriptorsSet.samplers.cend(), finderFunc);
            if (itr != descriptorsSet.samplers.cend())
            {
                printDescriptorDesc(*itr, "\t");
                printArrayDefs(itr->data.data,"\t");
                continue;
            }
        }
    }
}

// Pipeline shader stage processor impl

PipelineShaderStageProcessor::PipelineShaderStageProcessor(const std::vector<ShaderReflectionProcessor*>& shaderReflections
    , std::string refFilePath, std::string shaderFilePath)
    : shaderStages(shaderReflections)
    , reflectionFile(refFilePath)
    , shaderFile(shaderFilePath)
    , reflectedData()
    , allShaderCodes()
{}

void PipelineShaderStageProcessor::processReflections()
{
    // Maps each shader specialization const index to specialization const SpirV-cross ID
    std::vector<std::map<uint32_t, uint32_t>> specConstsMaps;
    processStages(specConstsMaps);
    
    processPipelineIO();
    processDescriptorsSets(specConstsMaps);
    processPushConstants(specConstsMaps);
}

void PipelineShaderStageProcessor::writeOutput()
{
    writeMergedShader();

    ShaderArchive archive;
    archive << reflectedData;

    if (CommonFunctions::writeToFile(reflectionFile, archive.archiveData()))
    {
        printf("Written shader reflections to %s\n", reflectionFile.c_str());
    }
}

bool PipelineShaderStageProcessor::crossCheckWrittenData()
{
    bool bIsSuccess = true;
    std::vector<unsigned char> reflectionData;
    ShaderReflected reflectedShader;
    if (CommonFunctions::readFromFile(reflectionFile, reflectionData))
    {
        ShaderArchive archive(reflectionData);
        archive << reflectedShader;
        std::vector<unsigned char> shaderReadData;

        if (CommonFunctions::readFromFile(shaderFile, shaderReadData))
        {
            std::vector<uint32_t> allShaderCode(shaderReadData.size() / 4);
            memcpy(allShaderCode.data(), shaderReadData.data(), shaderReadData.size());

            for (uint32_t i = 0; i < reflectedShader.stages.size(); ++i)
            {
                ShaderReflectionProcessor processor(allShaderCode, reflectedShader.stages[i].codeView);
                ShaderStageDescription stageDescRead = processor.getStageDesc();
                ShaderStageDescription stageDesc = shaderStages[i]->getStageDesc();
                if (stageDescRead.stage != stageDesc.stage || stageDescRead.entryPoint != stageDesc.entryPoint
                    || stageDescRead.pipelineBindPoint != stageDescRead.pipelineBindPoint || stageDescRead.codeView.size != stageDesc.codeView.size
                    || stageDescRead.codeView.startIdx != stageDesc.codeView.startIdx)
                {
                    printf("Rereading binary written file failed Reflection file %s shader file %s \n", reflectionFile.c_str(), shaderFile.c_str());
                    bIsSuccess = false;
                }
                else
                {
                    printf("Successfully parsed written shader %s and reflection %s\n", shaderFile.c_str(), reflectionFile.c_str());
                    bIsSuccess = bIsSuccess && true;
                }
            }
        }
    }
    return bIsSuccess;
}

void PipelineShaderStageProcessor::processStages(std::vector<std::map<uint32_t, uint32_t>>& specConstsMaps)
{
    using namespace SPIRV_CROSS_NAMESPACE;

    specConstsMaps.resize(shaderStages.size());
    for (int i = 0; i < shaderStages.size(); ++i)
    {
        ShaderReflectionProcessor* shaderStage = shaderStages[i];
        printf("Shader %s\n", shaderStage->shaderFileName.c_str());
        uint32_t startIndex = uint32_t(allShaderCodes.size());
        shaderStage->injectShaderCode(allShaderCodes);
        shaderStage->setCodeView(startIndex, uint32_t(allShaderCodes.size() - startIndex));
        printf("\tInjected shader code of size %d from %d index\n", shaderStage->codeView.size, shaderStage->codeView.startIdx);

        ShaderStageDescription stageDesc = shaderStage->getStageDesc();

        SmallVector<SpecializationConstant> specializationConsts = shaderStage->compiledData->get_specialization_constants();
        stageDesc.stageSpecializationEntries.resize(specializationConsts.size());
        for (uint32_t constIdx = 0; constIdx < specializationConsts.size(); ++constIdx)
        {
            const SPIRConstant& specEntryType = shaderStage->compiledData->get_constant(specializationConsts[constIdx].id);
            const SPIRType& typeRef = shaderStage->compiledData->get_type(specEntryType.constant_type);

            if (validateSpecializationConst(specEntryType, typeRef))
            {
                ReflectSpecializationConstant reflectConst;
                reflectConst.attributeName = shaderStage->compiledData->get_name(specializationConsts[constIdx].id);
                reflectConst.data.constantId = specializationConsts[constIdx].constant_id;
                reflectConst.data.type = getReflectPrimitiveType(typeRef.basetype);
                setSpecializationConstDefault(reflectConst.data.defaultValue, specEntryType, typeRef);

                specConstsMaps[i][uint32_t(specializationConsts[constIdx].id)] = constIdx;
                stageDesc.stageSpecializationEntries[constIdx] = reflectConst;

                printf("\tSpecialization constant %s Type ID %d , Primitive type %d\n", reflectConst.attributeName.c_str()
                    , uint32_t(specializationConsts[constIdx].id), reflectConst.data.type);
            }
        }

        reflectedData.stages.push_back(stageDesc);
    }
}

void PipelineShaderStageProcessor::processPipelineIO()
{
    using namespace SPIRV_CROSS_NAMESPACE;

    ShaderReflectionProcessor* vertexStage = nullptr;
    ShaderReflectionProcessor* fragStage = nullptr;

    for (uint32_t i = 0; i < reflectedData.stages.size(); ++i)
    {
        if (reflectedData.stages[i].stage == ShaderReflectionProcessor::VERTEX_STAGE && vertexStage == nullptr)
        {
            vertexStage = shaderStages[i];
        }
        else if (reflectedData.stages[i].stage == ShaderReflectionProcessor::FRAGMENT_STAGE && fragStage == nullptr)
        {
            fragStage = shaderStages[i];
        }
    }

    if (vertexStage)
    {
        for (Resource& resource : vertexStage->compiledData->get_shader_resources().stage_inputs)
        {
            const SPIRType& baseType = vertexStage->compiledData->get_type(resource.base_type_id);

            ReflectInputOutput reflectedInput;
            reflectedInput.attributeName = resource.name;
            reflectedInput.data.location = vertexStage->compiledData->get_decoration(resource.id, spv::Decoration::DecorationLocation);
            reflectedInput.data.type = getReflectedType(baseType);

            printf("Input : %s\n\tLocation : %d\n\tPrimitive type %d[%d][%d]\n", reflectedInput.attributeName.c_str()
                , reflectedInput.data.location, reflectedInput.data.type.primitive, reflectedInput.data.type.vecSize
                , reflectedInput.data.type.colSize);

            reflectedData.inputs.push_back(reflectedInput);
        }
    }
    if (fragStage)
    {
        for (Resource& resource : fragStage->compiledData->get_shader_resources().stage_outputs)
        {
            const SPIRType& baseType = fragStage->compiledData->get_type(resource.base_type_id);

            ReflectInputOutput reflectedOutput;
            reflectedOutput.attributeName = resource.name;
            reflectedOutput.data.location = fragStage->compiledData->get_decoration(resource.id, spv::Decoration::DecorationLocation);
            reflectedOutput.data.type = getReflectedType(baseType);

            printf("Output : %s\n\tLocation : %d\n", reflectedOutput.attributeName.c_str()
                , reflectedOutput.data.location);
            printReflectedType(reflectedOutput.data.type, "\t");

            reflectedData.outputs.push_back(reflectedOutput);
        }
    }
}

void PipelineShaderStageProcessor::processDescriptorsSets(const std::vector<std::map<uint32_t, uint32_t>>& specConstsMaps)
{
    using namespace SPIRV_CROSS_NAMESPACE;
    std::map<uint32_t, ReflectDescriptorBody> descriptorsSets;
    std::map<uint32_t, std::set<uint32_t>> descriptorSetsBinding;// List of all bindings used in each descriptors set

    for (uint32_t i = 0; i < reflectedData.stages.size(); ++i)
    {
        ShaderReflectionProcessor* shaderStage = shaderStages[i];
        const ShaderResources& resources = shaderStage->compiledData->get_shader_resources();
        EntryPoint& entryPoint = shaderStage->compiledData->get_entry_points_and_stages()[0];

        // Sampler Sampled texture and Texel samplerBuffers
        for (const Resource& resource : resources.sampled_images)
        {
            const SPIRType& baseType = shaderStage->compiledData->get_type(resource.base_type_id);
            const SPIRType& type = shaderStage->compiledData->get_type(resource.type_id);
            uint32_t set = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationBinding);
            descriptorSetsBinding[set].insert(binding);

            if (baseType.image.dim == spv::Dim::DimBuffer)
            {
                DescEntryTexelBuffer samplerBufferDesc;
                samplerBufferDesc.attributeName = resource.name;
                samplerBufferDesc.data.binding = binding;
                samplerBufferDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
                samplerBufferDesc.data.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                fillBufferFieldArrayInfo(samplerBufferDesc.data.data.arraySize, type, specConstsMaps[i], i);
                fillSampledImageFormats(samplerBufferDesc.data.data.format, shaderStage->compiledData->get_type(baseType.image.type));

                descriptorsSets[set].samplerBuffers.push_back(samplerBufferDesc);
            }
            else
            {
                DescEntryTexture sampledImageDesc;
                sampledImageDesc.attributeName = resource.name;
                sampledImageDesc.data.binding = binding;
                sampledImageDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
                sampledImageDesc.data.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                fillBufferFieldArrayInfo(sampledImageDesc.data.data.arraySize, type, specConstsMaps[i], i);
                sampledImageDesc.data.data.imageViewType = ShaderReflectionProcessor::imageViewType(baseType.image.dim, baseType.image.arrayed);
                fillSampledImageFormats(sampledImageDesc.data.data.format, shaderStage->compiledData->get_type(baseType.image.type));
                sampledImageDesc.data.data.bIsMultiSampled = baseType.image.ms;

                descriptorsSets[set].sampledTexAndArrays.push_back(sampledImageDesc);
            }
        }

        // Separate texture and Texel samplerBuffers(Though texel sampler buffer was not included in this list, still including because it was meant to be here as per documentation) 
        for (const Resource& resource : resources.separate_images)
        {
            const SPIRType& baseType = shaderStage->compiledData->get_type(resource.base_type_id);
            const SPIRType& type = shaderStage->compiledData->get_type(resource.type_id);
            uint32_t set = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationBinding);
            descriptorSetsBinding[set].insert(binding);

            if (baseType.image.dim == spv::Dim::DimBuffer)
            {
                DescEntryTexelBuffer samplerBufferDesc;
                samplerBufferDesc.attributeName = resource.name;
                samplerBufferDesc.data.binding = binding;
                samplerBufferDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
                samplerBufferDesc.data.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                fillBufferFieldArrayInfo(samplerBufferDesc.data.data.arraySize, type, specConstsMaps[i], i);
                fillSampledImageFormats(samplerBufferDesc.data.data.format, shaderStage->compiledData->get_type(baseType.image.type));

                descriptorsSets[set].samplerBuffers.push_back(samplerBufferDesc);
            }
            else
            {
                DescEntryTexture sampledImageDesc;
                sampledImageDesc.attributeName = resource.name;
                sampledImageDesc.data.binding = binding;
                sampledImageDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
                sampledImageDesc.data.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                fillBufferFieldArrayInfo(sampledImageDesc.data.data.arraySize, type, specConstsMaps[i], i);
                sampledImageDesc.data.data.imageViewType = ShaderReflectionProcessor::imageViewType(baseType.image.dim, baseType.image.arrayed);
                fillSampledImageFormats(sampledImageDesc.data.data.format, shaderStage->compiledData->get_type(baseType.image.type));
                sampledImageDesc.data.data.bIsMultiSampled = baseType.image.ms;

                descriptorsSets[set].textureAndArrays.push_back(sampledImageDesc);
            }
        }

        // Storage images and storage texel imageBuffers
        for (const Resource& resource : resources.storage_images)
        {
            const SPIRType& baseType = shaderStage->compiledData->get_type(resource.base_type_id);
            const SPIRType& type = shaderStage->compiledData->get_type(resource.type_id);
            uint32_t set = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationBinding);
            descriptorSetsBinding[set].insert(binding);

            if (baseType.image.dim == spv::Dim::DimBuffer)
            {
                DescEntryTexelBuffer samplerBufferDesc;
                samplerBufferDesc.attributeName = resource.name;
                samplerBufferDesc.data.binding = binding;
                samplerBufferDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
                samplerBufferDesc.data.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                fillBufferFieldArrayInfo(samplerBufferDesc.data.data.arraySize, type, specConstsMaps[i], i);
                samplerBufferDesc.data.data.format = ShaderReflectionProcessor::texelFormat(baseType.image.format);

                descriptorsSets[set].imageBuffers.push_back(samplerBufferDesc);
            }
            else
            {
                DescEntryTexture sampledImageDesc;
                sampledImageDesc.attributeName = resource.name;
                sampledImageDesc.data.binding = binding;
                sampledImageDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
                sampledImageDesc.data.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                fillBufferFieldArrayInfo(sampledImageDesc.data.data.arraySize, type, specConstsMaps[i], i);
                sampledImageDesc.data.data.imageViewType = ShaderReflectionProcessor::imageViewType(baseType.image.dim, baseType.image.arrayed);
                sampledImageDesc.data.data.format = ShaderReflectionProcessor::texelFormat(baseType.image.format);
                sampledImageDesc.data.data.bIsMultiSampled = baseType.image.ms;

                descriptorsSets[set].imagesAndImgArrays.push_back(sampledImageDesc);
            }
        }

        // Input attachments
        for (const Resource& resource : resources.subpass_inputs)
        {
            const SPIRType& type = shaderStage->compiledData->get_type(resource.type_id);
            uint32_t set = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationBinding);
            descriptorSetsBinding[set].insert(binding);

            DescEntrySubpassInput samplerDesc;
            samplerDesc.attributeName = resource.name;
            samplerDesc.data.binding = binding;
            samplerDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
            samplerDesc.data.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            samplerDesc.data.data = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationInputAttachmentIndex);

            descriptorsSets[set].subpassInputs.push_back(samplerDesc);
        }

        // Samplers
        for (const Resource& resource : resources.separate_samplers)
        {
            const SPIRType& type = shaderStage->compiledData->get_type(resource.type_id);
            uint32_t set = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationBinding);
            descriptorSetsBinding[set].insert(binding);

            DescEntrySampler samplerDesc;
            samplerDesc.attributeName = resource.name;
            samplerDesc.data.binding = binding;
            samplerDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
            samplerDesc.data.type = VK_DESCRIPTOR_TYPE_SAMPLER;
            fillBufferFieldArrayInfo(samplerDesc.data.data, type, specConstsMaps[i], i);

            descriptorsSets[set].samplers.push_back(samplerDesc);
        }

        // Uniform buffers
        for (const Resource& resource : resources.uniform_buffers)
        {
            const SPIRType& baseType = shaderStage->compiledData->get_type(resource.base_type_id);
            uint32_t set = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationBinding);
            descriptorSetsBinding[set].insert(binding);

            DescEntryBuffer bufferDesc;
            bufferDesc.attributeName = shaderStage->compiledData->get_name(resource.id);
            bufferDesc.data.binding = binding;
            bufferDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
            bufferDesc.data.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            fillBufferFields(bufferDesc.data.data, baseType, shaderStage->compiledData, specConstsMaps[i], i);

            descriptorsSets[set].uniforms.push_back(bufferDesc);
        }

        // Storage buffers
        for (const Resource& resource : resources.storage_buffers)
        {
            const SPIRType& baseType = shaderStage->compiledData->get_type(resource.base_type_id);
            uint32_t set = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = shaderStage->compiledData->get_decoration(resource.id, spv::DecorationBinding);
            descriptorSetsBinding[set].insert(binding);

            DescEntryBuffer bufferDesc;
            bufferDesc.attributeName = shaderStage->compiledData->get_name(resource.id);
            bufferDesc.data.binding = binding;
            bufferDesc.data.stagesUsed = ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);
            bufferDesc.data.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            fillBufferFields(bufferDesc.data.data, baseType, shaderStage->compiledData, specConstsMaps[i], i);

            descriptorsSets[set].buffers.push_back(bufferDesc);
        }
    }

    reflectedData.descriptorsSets.resize(descriptorsSets.size());
    uint32_t idx = 0;
    for (auto itr = descriptorsSets.begin(); itr != descriptorsSets.end(); ++idx, ++itr)
    {
        reflectedData.descriptorsSets[idx] = itr->second;
        reflectedData.descriptorsSets[idx].set = itr->first;
        reflectedData.descriptorsSets[idx].usedBindings.assign(descriptorSetsBinding[itr->first].cbegin()
            , descriptorSetsBinding[itr->first].cend());
        sort(reflectedData.descriptorsSets[idx].usedBindings.begin(), reflectedData.descriptorsSets[idx].usedBindings.end());

        squashDuplicates(reflectedData.descriptorsSets[idx]);
        reflectedData.descriptorsSets[idx].combinedSetUsage = combinedDescritorsUsage(reflectedData.descriptorsSets[idx]);
        printDescriptorsSet(reflectedData.descriptorsSets[idx]);
    }
}

void PipelineShaderStageProcessor::processPushConstants(const std::vector<std::map<uint32_t, uint32_t>>& specConstsMaps)
{
    using namespace SPIRV_CROSS_NAMESPACE;
    reflectedData.pushConstants.data.pushConstantField.stride = 0;
    reflectedData.pushConstants.data.stagesUsed = 0;
    for (uint32_t i = 0; i < reflectedData.stages.size(); ++i)
    {
        ShaderReflectionProcessor* shaderStage = shaderStages[i];
        const ShaderResources& resources = shaderStage->compiledData->get_shader_resources();
        EntryPoint& entryPoint = shaderStage->compiledData->get_entry_points_and_stages()[0];

        if (resources.push_constant_buffers.size() == 1)
        {
            const Resource& resource = resources.push_constant_buffers[0];
            reflectedData.pushConstants.attributeName = shaderStage->compiledData->get_name(resource.id);
            reflectedData.pushConstants.data.stagesUsed |= ShaderReflectionProcessor::shaderStageFlag(entryPoint.execution_model);

            const SPIRV_CROSS_NAMESPACE::SPIRType& basetype = shaderStage->compiledData->get_type(resource.base_type_id);
            fillBufferFields(reflectedData.pushConstants.data.pushConstantField, basetype, shaderStage->compiledData, specConstsMaps[i], i);
        }
    }

    squashDuplicates(reflectedData.pushConstants.data.pushConstantField);

    printf("PushConstant : %s\n\tStages used : %d\n", reflectedData.pushConstants.attributeName.c_str(),reflectedData.pushConstants.data.stagesUsed);
    printFields(reflectedData.pushConstants.data.pushConstantField,"\t");
}

void PipelineShaderStageProcessor::writeMergedShader()
{
    if (allShaderCodes.empty())
    {
        printf("Merged shader code is empty! Skipping writing to %s\n", shaderFile.c_str());
        return;
    }

    if (CommonFunctions::writeToFile(shaderFile, allShaderCodes))
    {
        printf("Written all shaders to %s\n", shaderFile.c_str());
    }
}