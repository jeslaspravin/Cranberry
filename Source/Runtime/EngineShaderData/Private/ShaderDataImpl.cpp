/*!
 * \file ShaderDataImpl.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ShaderArchive.h"
#include "ShaderReflected.h"

//////////////////////////////////////////////////////////////////////////
//// Archive function implementations
//////////////////////////////////////////////////////////////////////////

template <typename ArchiveType>
std::enable_if_t<!std::is_pointer_v<ArchiveType>, void> operator<<(ShaderArchive &archive, ArchiveType &typeData)
{
    uint32_t currentPtr = archive.archivePtr;
    if (archive.moveFwd(sizeof(decltype(typeData))))
    {
        if (archive.isLoading())
        {
            memcpy(&typeData, &archive.archive[currentPtr], sizeof(decltype(typeData)));
        }
        else
        {
            memcpy(&archive.archive[currentPtr], &typeData, sizeof(decltype(typeData)));
        }
    }
    else
    {
        archive.status = false;
    }
}

template <>
void operator<< <std::string>(ShaderArchive &archive, std::string &typeData)
{
    uint32_t currentPtr = archive.archivePtr;

    if (archive.isLoading())
    {
        uint32_t indexOffset = 0;
        for (; archive.archive[currentPtr + indexOffset] != '\0'; ++indexOffset)
            ;

        typeData.resize(indexOffset);
        if (archive.moveFwd(uint32_t(typeData.length() + 1))) // Adding 1 here as offset starts from 0
        {
            memcpy(&typeData[0], &archive.archive[currentPtr], typeData.size());
        }
        else
        {
            archive.status = false;
        }
    }
    else
    {
        if (archive.moveFwd(uint32_t(typeData.length() + 1))) // Adding 1 here to account for null char
        {
            memcpy(&archive.archive[currentPtr], typeData.c_str(), typeData.length() + 1);
        }
        else
        {
            archive.status = false;
        }
    }
}

template <>
void operator<< <ShaderStageDescription>(ShaderArchive &archive, ShaderStageDescription &typeData)
{
    archive << typeData.stage;
    archive << typeData.pipelineBindPoint;
    archive << typeData.entryPoint;
    archive << typeData.codeView;
    archive << typeData.stageSpecializationEntries;
}

template <>
void operator<< <PushConstantEntry>(ShaderArchive &archive, PushConstantEntry &typeData)
{
    archive << typeData.stagesUsed;
    archive << typeData.pushConstantField;
}

template <>
void operator<< <ReflectBufferShaderField>(ShaderArchive &archive, ReflectBufferShaderField &typeData)
{
    archive << typeData.stride;
    archive << typeData.bufferFields;
    archive << typeData.bufferStructFields;
}

template <>
void operator<< <ReflectTexelBufferShaderField>(ShaderArchive &archive, ReflectTexelBufferShaderField &typeData)
{
    archive << typeData.arraySize;
    archive << typeData.format;
}

template <>
void operator<< <ReflectTextureShaderField>(ShaderArchive &archive, ReflectTextureShaderField &typeData)
{
    archive << typeData.imageViewType;
    archive << typeData.arraySize;
    archive << typeData.format;
    archive << typeData.bIsMultiSampled;
}

template <>
void operator<< <ReflectDescriptorBody>(ShaderArchive &archive, ReflectDescriptorBody &typeData)
{
    archive << typeData.set;
    archive << typeData.usedBindings;
    archive << typeData.combinedSetUsage;

    archive << typeData.uniforms;
    archive << typeData.buffers;
    archive << typeData.samplerBuffers;
    archive << typeData.imageBuffers;
    archive << typeData.sampledTexAndArrays;
    archive << typeData.textureAndArrays;
    archive << typeData.subpassInputs;
    archive << typeData.imagesAndImgArrays;
    archive << typeData.samplers;
}

template <>
void operator<< <ShaderReflected>(ShaderArchive &archive, ShaderReflected &typeData)
{
    archive << typeData.stages;
    archive << typeData.inputs;
    archive << typeData.outputs;
    archive << typeData.descriptorsSets;
    archive << typeData.pushConstants;
}

template <typename Type>
std::enable_if_t<std::is_integral_v<Type>, void> operator<<(ShaderArchive &archive, std::vector<Type> &typeData)
{
    uint32_t dataSize = uint32_t(typeData.size());
    archive << dataSize;

    if (dataSize == 0)
    {
        return;
    }

    uint32_t currentPtr = archive.archivePtr;
    if (!archive.moveFwd(dataSize * sizeof(decltype(typeData[0]))))
    {
        archive.status = false;
        return;
    }

    if (archive.isLoading())
    {
        typeData.resize(dataSize);
        memcpy(typeData.data(), &archive.archive[currentPtr], dataSize * sizeof(decltype(typeData[0])));
    }
    else
    {
        memcpy(&archive.archive[currentPtr], typeData.data(), dataSize * sizeof(decltype(typeData[0])));
    }
}

template <typename Type>
std::enable_if_t<!std::is_integral_v<Type>, void> operator<<(ShaderArchive &archive, std::vector<Type> &typeData)
{
    uint32_t dataSize = uint32_t(typeData.size());
    archive << dataSize;

    if (archive.isLoading())
    {
        typeData.resize(dataSize);
    }

    for (uint32_t i = 0; i < dataSize; ++i)
    {
        archive << typeData[i];
    }
}

template <typename Type>
void operator<<(ShaderArchive &archive, NamedAttribute<Type> &typeData)
{
    archive << typeData.attributeName;
    archive << typeData.data;
}

template <typename Type>
void operator<<(ShaderArchive &archive, StructInnerFields<Type> &typeData)
{
    archive << typeData.offset;
    archive << typeData.stride;
    archive << typeData.totalSize;
    archive << typeData.arraySize;
    archive << typeData.data;
}

template <typename Type>
void operator<<(ShaderArchive &archive, DescriptorSetEntry<Type> &typeData)
{
    archive << typeData.readWriteState;
    archive << typeData.binding;
    archive << typeData.stagesUsed;
    archive << typeData.type;
    archive << typeData.data;
}