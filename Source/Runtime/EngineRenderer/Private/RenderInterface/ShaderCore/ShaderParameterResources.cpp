/*!
 * \file ShaderParameterResources.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <unordered_set>

#include "Math/Matrix4.h"
#include "Math/Vector2D.h"
#include "Math/Vector4D.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "IRenderInterfaceModule.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"
#include "ShaderReflected.h"

void ShaderDescriptorParamType::wrapReflectedDescriptors(
    std::map<StringID, ShaderDescriptorParamType *> &descriptorParams, const ReflectDescriptorBody &reflectDescriptors,
    std::map<StringID, ShaderBufferDescriptorType *> *filterBufferDescriptors /*= nullptr*/
)
{
    for (const DescEntryBuffer &descriptorInfo : reflectDescriptors.uniforms)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderBufferDescriptorType *bufferDescriptorWrapper = new ShaderBufferDescriptorType();
        bufferDescriptorWrapper->bIsStorage = false;
        bufferDescriptorWrapper->bufferEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = bufferDescriptorWrapper;
        if (filterBufferDescriptors)
        {
            (*filterBufferDescriptors)[attribName] = bufferDescriptorWrapper;
        }
    }
    for (const DescEntryBuffer &descriptorInfo : reflectDescriptors.buffers)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderBufferDescriptorType *bufferDescriptorWrapper = new ShaderBufferDescriptorType();
        bufferDescriptorWrapper->bIsStorage = true;
        bufferDescriptorWrapper->bufferEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = bufferDescriptorWrapper;
        if (filterBufferDescriptors)
        {
            (*filterBufferDescriptors)[attribName] = bufferDescriptorWrapper;
        }
    }
    for (const DescEntryTexelBuffer &descriptorInfo : reflectDescriptors.imageBuffers)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderBufferDescriptorType *texelDescriptorWrapper = new ShaderBufferDescriptorType();
        texelDescriptorWrapper->bIsStorage = true;
        texelDescriptorWrapper->texelBufferEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = texelDescriptorWrapper;
    }
    for (const DescEntryTexelBuffer &descriptorInfo : reflectDescriptors.samplerBuffers)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderBufferDescriptorType *texelDescriptorWrapper = new ShaderBufferDescriptorType();
        texelDescriptorWrapper->bIsStorage = false;
        texelDescriptorWrapper->texelBufferEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = texelDescriptorWrapper;
    }
    for (const DescEntryTexture &descriptorInfo : reflectDescriptors.imagesAndImgArrays)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderTextureDescriptorType *textureDescriptorWrapper = new ShaderTextureDescriptorType();
        textureDescriptorWrapper->bIsAttachedSampler = false;
        textureDescriptorWrapper->imageUsageFlags = EImageShaderUsage::Writing;
        textureDescriptorWrapper->textureEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = textureDescriptorWrapper;
    }
    for (const DescEntryTexture &descriptorInfo : reflectDescriptors.textureAndArrays)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderTextureDescriptorType *textureDescriptorWrapper = new ShaderTextureDescriptorType();
        textureDescriptorWrapper->bIsAttachedSampler = false;
        textureDescriptorWrapper->imageUsageFlags = EImageShaderUsage::Sampling;
        textureDescriptorWrapper->textureEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = textureDescriptorWrapper;
    }
    for (const DescEntryTexture &descriptorInfo : reflectDescriptors.sampledTexAndArrays)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderTextureDescriptorType *textureDescriptorWrapper = new ShaderTextureDescriptorType();
        textureDescriptorWrapper->bIsAttachedSampler = true;
        textureDescriptorWrapper->imageUsageFlags = EImageShaderUsage::Sampling;
        textureDescriptorWrapper->textureEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = textureDescriptorWrapper;
    }
    for (const DescEntrySampler &descriptorInfo : reflectDescriptors.samplers)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        ShaderSamplerDescriptorType *samplerDescriptorWrapper = new ShaderSamplerDescriptorType();
        samplerDescriptorWrapper->samplerEntryPtr = &descriptorInfo;
        descriptorParams[attribName] = samplerDescriptorWrapper;
    }
    for (const DescEntrySubpassInput &descriptorInfo : reflectDescriptors.subpassInputs)
    {
        StringID attribName{ descriptorInfo.attributeName.c_str() };
        LOG_WARN("DescriptorTypeParams", "Sub pass inputs are not supported yet %s", descriptorInfo.attributeName.c_str());
    }
}

template <typename DescriptorParamType>
const DescriptorParamType *Cast(const ShaderDescriptorParamType *shaderDescriptorType)
{
    if (DescriptorParamType::PARAM_TYPE == shaderDescriptorType->paramType)
    {
        return static_cast<const DescriptorParamType *>(shaderDescriptorType);
    }
    return nullptr;
}

template <typename DescriptorParamType>
DescriptorParamType *Cast(ShaderDescriptorParamType *shaderDescriptorType)
{
    if (DescriptorParamType::PARAM_TYPE == shaderDescriptorType->paramType)
    {
        return static_cast<DescriptorParamType *>(shaderDescriptorType);
    }
    return nullptr;
}

ShaderTextureDescriptorType::ShaderTextureDescriptorType() { paramType = ShaderDescriptorParamType::Texture; }
ShaderBufferDescriptorType::ShaderBufferDescriptorType() { paramType = ShaderDescriptorParamType::Buffer; }
ShaderSamplerDescriptorType::ShaderSamplerDescriptorType() { paramType = ShaderDescriptorParamType::Sampler; }

//////////////////////////////////////////////////////////////////////////
// ShaderSetParametersLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderSetParametersLayout)

ShaderSetParametersLayout::ShaderSetParametersLayout(const ShaderResource *shaderResource, uint32 setID)
    : respectiveShaderRes(shaderResource)
    , shaderSetID(setID)
    , bHasBindless(false)
{}

void ShaderSetParametersLayout::init()
{
    BaseType::init();
    const ShaderReflected *shaderReflection = respectiveShaderRes->getReflection();

    std::map<StringID, ShaderBufferDescriptorType *> bufferDescriptors;
    for (const ReflectDescriptorBody &descriptorsSet : shaderReflection->descriptorsSets)
    {
        if (descriptorsSet.set == shaderSetID)
        {
            ShaderDescriptorParamType::wrapReflectedDescriptors(paramsLayout, descriptorsSet, &bufferDescriptors);
        }
    }

    bindBufferParamInfo(bufferDescriptors);
    std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
    {
        std::map<StringID, SpecializationConstantEntry> specConsts;
        respectiveShaderRes->getSpecializationConsts(specConsts);
        ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderReflection);
    }

    // Fill those bound buffer info with GPU reflect data
    for (const std::pair<const StringID, ShaderBufferDescriptorType *> &bufferDescWrapper : bufferDescriptors)
    {
        ShaderParameterUtility::fillRefToBufParamInfo(
            *bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data, specializationConsts
        );
    }
}

void ShaderSetParametersLayout::release()
{
    for (const std::pair<const StringID, ShaderDescriptorParamType *> &shaderDescriptorTypeWrapper : paramsLayout)
    {
        delete shaderDescriptorTypeWrapper.second;
    }
    paramsLayout.clear();

    BaseType::release();
}

const ShaderDescriptorParamType *ShaderSetParametersLayout::parameterDescription(StringID paramName) const
{
    uint32 temp;
    return parameterDescription(temp, paramName);
}

const ShaderDescriptorParamType *ShaderSetParametersLayout::parameterDescription(uint32 &outSetIdx, StringID paramName) const
{
    auto foundParamItr = paramsLayout.find(paramName);
    if (foundParamItr != paramsLayout.cend())
    {
        outSetIdx = shaderSetID;
        return foundParamItr->second;
    }
    LOG_ERROR(
        "ShaderSetParametersLayout", "Parameter %s is not available in shader %s at set %u", paramName, respectiveShaderRes->getResourceName(),
        shaderSetID
    );
    return nullptr;
}

const std::map<StringID, ShaderDescriptorParamType *> &ShaderSetParametersLayout::allParameterDescriptions() const { return paramsLayout; }

//////////////////////////////////////////////////////////////////////////
// ShaderParametersLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderParametersLayout)

ShaderParametersLayout::ShaderParametersLayout(const ShaderResource *shaderResource)
    : respectiveShaderRes(shaderResource)
{}

void ShaderParametersLayout::init()
{
    BaseType::init();

    const ShaderReflected *shaderReflection = respectiveShaderRes->getReflection();

    std::map<uint32, std::map<StringID, ShaderDescriptorParamType *>> setToParamsLayout;

    // Wrapping descriptors sets reflected info into ShaderDescriptorParamType wrappers
    std::map<StringID, ShaderBufferDescriptorType *> bufferDescriptors;
    for (const ReflectDescriptorBody &descriptorsSet : shaderReflection->descriptorsSets)
    {
        ShaderDescriptorParamType::wrapReflectedDescriptors(setToParamsLayout[descriptorsSet.set], descriptorsSet, &bufferDescriptors);
    }

    // Fill those bound buffer info with GPU reflect data
    respectiveShaderRes->bindBufferParamInfo(bufferDescriptors);
    std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
    {
        std::map<StringID, SpecializationConstantEntry> specConsts;
        respectiveShaderRes->getSpecializationConsts(specConsts);
        ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderReflection);
    }

    for (const std::pair<const StringID, ShaderBufferDescriptorType *> &bufferDescWrapper : bufferDescriptors)
    {
        ShaderParameterUtility::fillRefToBufParamInfo(
            *bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data, specializationConsts
        );
    }

    for (const std::pair<const uint32, std::map<StringID, ShaderDescriptorParamType *>> &setToDescriptorsPair : setToParamsLayout)
    {
        for (const std::pair<const StringID, ShaderDescriptorParamType *> &descriptorWrapper : setToDescriptorsPair.second)
        {
            // Since currently we support only one unique name per shader
            fatalAssertf(
                paramsLayout.find(descriptorWrapper.first) == paramsLayout.end(),
                "Shader descriptor param name must be unique for a shader pipeline"
            );
            paramsLayout.insert({
                descriptorWrapper.first, {setToDescriptorsPair.first, descriptorWrapper.second}
            });
        }
    }
}

void ShaderParametersLayout::release()
{
    for (const std::pair<const StringID, std::pair<uint32, ShaderDescriptorParamType *>> &shaderDescriptorTypeWrapper : paramsLayout)
    {
        delete shaderDescriptorTypeWrapper.second.second;
    }
    paramsLayout.clear();

    BaseType::release();
}

const ShaderDescriptorParamType *ShaderParametersLayout::parameterDescription(uint32 &outSetIdx, StringID paramName) const
{
    auto foundParamItr = paramsLayout.find(paramName);
    if (foundParamItr != paramsLayout.cend())
    {
        outSetIdx = foundParamItr->second.first;
        return foundParamItr->second.second;
    }
    LOG_ERROR("ShaderParametersLayout", "Parameter %s is not available in shader %s", paramName, respectiveShaderRes->getResourceName());
    return nullptr;
}

const ShaderDescriptorParamType *ShaderParametersLayout::parameterDescription(StringID paramName) const
{
    uint32 temp;
    return parameterDescription(temp, paramName);
}

std::map<StringID, ShaderDescriptorParamType *> ShaderParametersLayout::allParameterDescriptions() const
{
    std::map<StringID, ShaderDescriptorParamType *> allParamsLayout;
    for (const std::pair<const StringID, std::pair<uint32, ShaderDescriptorParamType *>> &paramLayout : paramsLayout)
    {
        allParamsLayout[paramLayout.first] = paramLayout.second.second;
    }
    return allParamsLayout;
}

uint32 ShaderParametersLayout::getSetID(StringID paramName) const
{
    auto foundParamItr = paramsLayout.find(paramName);
    fatalAssertf(
        foundParamItr != paramsLayout.cend(), "Cannot call this function with invalid param name, Use "
                                              "parameterDescription function if validity is not sure"
    );
    return foundParamItr->second.first;
}

//////////////////////////////////////////////////////////////////////////
// ShaderParameters
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderParameters)

size_t ShaderParameters::BufferParameterUpdate::Hasher::operator()(const BufferParameterUpdate &keyVal) const noexcept
{
    size_t seed = HashUtility::hash(keyVal.bufferName);
    HashUtility::hashCombine(seed, keyVal.paramName);
    HashUtility::hashCombine(seed, keyVal.index);
    return seed;
}

ShaderParameters::ShaderParameters(const GraphicsResource *shaderParamLayout, const std::set<uint32> &ignoredSetIds /* = {}*/)
    : GraphicsResource()
    , paramLayout(shaderParamLayout)
    , ignoredSets(ignoredSetIds)
{
    if (paramLayout->getType()->isChildOf<ShaderSetParametersLayout>())
    {
        std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
        {
            const ShaderResource *shaderRes = static_cast<const ShaderSetParametersLayout *>(paramLayout)->getShaderResource();
            std::map<StringID, SpecializationConstantEntry> specConsts;
            shaderRes->getSpecializationConsts(specConsts);
            ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderRes->getReflection());
        }
        initParamsMaps(static_cast<const ShaderSetParametersLayout *>(paramLayout)->allParameterDescriptions(), specializationConsts);
    }
    else if (paramLayout->getType()->isChildOf<ShaderParametersLayout>())
    {
        std::map<StringID, ShaderDescriptorParamType *> allParameters
            = static_cast<const ShaderParametersLayout *>(paramLayout)->allParameterDescriptions();
        if (!ignoredSets.empty())
        {
            for (auto itr = allParameters.begin(); itr != allParameters.end();)
            {
                if (ignoredSets.find(static_cast<const ShaderParametersLayout *>(paramLayout)->getSetID(itr->first)) == ignoredSets.end())
                {
                    ++itr;
                }
                else
                {
                    allParameters.erase(itr++);
                }
            }
        }

        std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
        {
            const ShaderResource *shaderRes = static_cast<const ShaderParametersLayout *>(paramLayout)->getShaderResource();
            std::map<StringID, SpecializationConstantEntry> specConsts;
            shaderRes->getSpecializationConsts(specConsts);
            ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderRes->getReflection());
        }
        initParamsMaps(allParameters, specializationConsts);
    }
    else
    {
        fatalAssertf(false, "Unsupported Shader parameters layout");
    }
}

void ShaderParameters::addRef() { refCounter.fetch_add(1, std::memory_order::release); }

void ShaderParameters::removeRef()
{
    uint32 count = refCounter.fetch_sub(1, std::memory_order::acq_rel);
    if (count == 1)
    {
        ENQUEUE_COMMAND(DeleteShaderParameter)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                graphicsHelper->markForDeletion(graphicsInstance, this, EDeferredDelStrategy::SwapchainCount);
            }
        );
    }
}

uint32 ShaderParameters::refCount() const { return refCounter.load(std::memory_order::acquire); }

void ShaderParameters::init()
{
    BaseType::init();
    for (const std::pair<const StringID, BufferParametersData> &bufferParameters : shaderBuffers)
    {
        // Only if not using already set resource externally, Or already initialized
        if (bufferParameters.second.gpuBuffer.isValid() && !bufferParameters.second.gpuBuffer->isValid())
        {
            bufferParameters.second.gpuBuffer->setResourceName(
                getResourceName()
                + TCHAR("_") + UTF8_TO_TCHAR(bufferParameters.second.descriptorInfo->bufferEntryPtr->attributeName.c_str())
                );
            bufferParameters.second.gpuBuffer->init();
        }
    }
}

void ShaderParameters::initBufferParams(
    BufferParametersData &bufferParamData, const ShaderBufferParamInfo *bufferParamInfo, void *outerPtr, StringID outerName
) const
{
    for (const ShaderBufferField *currentField : *bufferParamInfo)
    {
        bufferParamData.bufferParams[StringID(currentField->paramName)] = { outerPtr, outerName, currentField };
        if (BIT_SET(currentField->fieldDecorations, ShaderBufferField::IsStruct))
        {
            // AoS inside shader base uniform struct is supported, AoSoA... not supported due to
            // parameter indexing limitation being 1 right now
            if (outerName.isValid() && currentField->isIndexAccessible())
            {
                fatalAssertf(!"We do not support nested array in parameters", "We do not support nested array in parameters");
            }
            void *nextOuterPtr = nullptr;
            // Not pointer or if pointer is set
            if (!currentField->isPointer() || *(reinterpret_cast<void **>(currentField->fieldPtr(outerPtr))) != nullptr)
            {
                nextOuterPtr = currentField->fieldData(outerPtr, nullptr, nullptr);
                initBufferParams(bufferParamData, currentField->paramInfo, nextOuterPtr, StringID(currentField->paramName));
            }
        }
    }
}

void ShaderParameters::initParamsMaps(
    const std::map<StringID, ShaderDescriptorParamType *> &paramsDesc,
    const std::vector<std::vector<SpecializationConstantEntry>> &specializationConsts
)
{
    for (const std::pair<const StringID, ShaderDescriptorParamType *> &paramDesc : paramsDesc)
    {
        if (const ShaderBufferDescriptorType *bufferParamDesc = Cast<ShaderBufferDescriptorType>(paramDesc.second))
        {
            if (bufferParamDesc->bufferEntryPtr != nullptr)
            {
                BufferParametersData paramData;
                paramData.descriptorInfo = bufferParamDesc;
                paramData.cpuBuffer = new uint8[bufferParamDesc->bufferParamInfo->paramNativeStride()];
                memset(paramData.cpuBuffer, 0, bufferParamDesc->bufferParamInfo->paramNativeStride());
                initBufferParams(paramData, bufferParamDesc->bufferParamInfo, paramData.cpuBuffer, StringID(EInitType::InitType_NoInit));

                uint32 bufferInitStride = bufferParamDesc->bufferParamInfo->paramStride();
                if (initRuntimeArrayData(paramData))
                {
                    bufferInitStride = paramData.runtimeArray->offset;
                    // If 0 runtime offset then it must be resized
                    if (bufferInitStride == 0)
                    {
                        LOG_DEBUG(
                            "ShaderParameters", "Runtime array \"%s\" struct has 0 size and must be resized before init",
                            paramData.runtimeArray->paramName
                        );
                    }
                }

                if (bufferInitStride > 0)
                {
                    const GraphicsHelperAPI *graphicsHelper = IRenderInterfaceModule::get()->currentGraphicsHelper();
                    IGraphicsInstance *graphicsInstance = IRenderInterfaceModule::get()->currentGraphicsInstance();
                    paramData.gpuBuffer = bufferParamDesc->bIsStorage
                                              ? graphicsHelper->createWriteOnlyBuffer(graphicsInstance, bufferInitStride)
                                              : graphicsHelper->createReadOnlyBuffer(graphicsInstance, bufferInitStride);
                }

                StringID attribName{ bufferParamDesc->bufferEntryPtr->attributeName.c_str() };
                shaderBuffers[attribName] = paramData;
            }
            else
            {
                debugAssert(bufferParamDesc->texelBufferEntryPtr->data.data.arraySize.size() == 1);
                uint32 count = ShaderParameterUtility::getArrayElementCount<1>(
                    paramDesc.first, bufferParamDesc->texelBufferEntryPtr->data.data.arraySize, specializationConsts
                );

                StringID attribName{ bufferParamDesc->texelBufferEntryPtr->attributeName.c_str() };
                TexelParameterData &paramData = shaderTexels[attribName];
                paramData.descriptorInfo = bufferParamDesc;
                paramData.gpuBuffers.resize(count, nullptr);
            }
        }
        else if (const ShaderTextureDescriptorType *textureParamDesc = Cast<ShaderTextureDescriptorType>(paramDesc.second))
        {
            debugAssert(textureParamDesc->textureEntryPtr->data.data.arraySize.size() == 1);
            uint32 count = ShaderParameterUtility::getArrayElementCount<1>(
                paramDesc.first, textureParamDesc->textureEntryPtr->data.data.arraySize, specializationConsts
            );

            StringID attribName{ textureParamDesc->textureEntryPtr->attributeName.c_str() };
            TextureParameterData &paramData = shaderTextures[attribName];
            paramData.textures.resize(count);
            paramData.descriptorInfo = textureParamDesc;
        }
        else if (const ShaderSamplerDescriptorType *samplerParamDesc = Cast<ShaderSamplerDescriptorType>(paramDesc.second))
        {
            debugAssert(samplerParamDesc->samplerEntryPtr->data.data.size() == 1);
            uint32 count = ShaderParameterUtility::getArrayElementCount<1>(
                paramDesc.first, samplerParamDesc->samplerEntryPtr->data.data, specializationConsts
            );

            StringID attribName{ samplerParamDesc->samplerEntryPtr->attributeName.c_str() };
            SamplerParameterData &paramData = shaderSamplers[attribName];
            paramData.samplers.resize(count);
            paramData.descriptorInfo = samplerParamDesc;
        }
    }
}

bool ShaderParameters::initRuntimeArrayData(BufferParametersData &bufferParamData) const
{
    uint32 runtimeOffset = 0;
    StringID bufferRuntimeParamName(EInitType::InitType_NoInit);
    uint32 paramsCount = 0;
    for (const ShaderBufferField *currentField : *bufferParamData.descriptorInfo->bufferParamInfo)
    {
        if (currentField->isPointer())
        {
            // More than one runtime per struct is not allowed
            debugAssert(!bufferRuntimeParamName.isValid());
            bufferRuntimeParamName = StringID(currentField->paramName);
            runtimeOffset = currentField->offset;
        }
        paramsCount++;
    }

    if (bufferRuntimeParamName.isValid())
    {
        // If any params then offset/stride cannot be 0
        debugAssert(paramsCount == 1 || runtimeOffset > 0);
        BufferParametersData::RuntimeArrayParameter runtimeParams{ bufferRuntimeParamName, runtimeOffset, 0 };
        bufferParamData.runtimeArray = std::move(runtimeParams);
        return true;
    }
    return false;
}

void ShaderParameters::release()
{
    BaseType::release();

    for (const std::pair<const StringID, BufferParametersData> &bufferParam : shaderBuffers)
    {
        delete[] bufferParam.second.cpuBuffer;
    }
    shaderBuffers.clear();
    shaderTexels.clear();
    shaderTextures.clear();
    shaderSamplers.clear();
}

String ShaderParameters::getResourceName() const { return descriptorSetName; }

void ShaderParameters::setResourceName(const String &name) { descriptorSetName = name; }

std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> ShaderParameters::getAllReadOnlyTextures() const
{
    // TODO(Jeslas) : Support image view
    std::unordered_set<ImageResourceRef> uniqueness;
    std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> textures;
    for (const std::pair<const StringID, TextureParameterData> &textuteParam : shaderTextures)
    {
        for (const auto &img : textuteParam.second.textures)
        {
            if (!img.texture.isValid() || !uniqueness.emplace(img.texture).second)
                continue;

            if (img.texture->isShaderRead()
                && (!img.texture->isShaderWrite()
                    || BIT_NOT_SET(textuteParam.second.descriptorInfo->textureEntryPtr->data.readWriteState, EDescriptorEntryState::WriteOnly)))
            {
                textures.emplace_back(std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>{ img.texture,
                                                                                                        textuteParam.second.descriptorInfo });
            }
        }
    }
    return textures;
}

std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> ShaderParameters::getAllReadOnlyBuffers() const
{
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> buffers;
    for (const std::pair<const StringID, BufferParametersData> &bufferParam : shaderBuffers)
    {
        if (!bufferParam.second.descriptorInfo->bIsStorage
            || BIT_NOT_SET(bufferParam.second.descriptorInfo->bufferEntryPtr->data.readWriteState, EDescriptorEntryState::WriteOnly))
        {
            buffers.emplace_back(std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>{ bufferParam.second.gpuBuffer,
                                                                                                   bufferParam.second.descriptorInfo });
        }
    }
    return buffers;
}

std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> ShaderParameters::getAllReadOnlyTexels() const
{
    // TODO(Jeslas) : Support texel view
    std::unordered_set<BufferResourceRef> uniqueness;

    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> buffers;
    for (const std::pair<const StringID, TexelParameterData> &bufferParam : shaderTexels)
    {
        for (BufferResourceRef texels : bufferParam.second.gpuBuffers)
        {
            if (!texels.isValid() || !uniqueness.emplace(texels).second)
                continue;

            if (!bufferParam.second.descriptorInfo->bIsStorage
                || BIT_NOT_SET(bufferParam.second.descriptorInfo->texelBufferEntryPtr->data.readWriteState, EDescriptorEntryState::WriteOnly))
            {
                buffers.emplace_back(std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>{ texels,
                                                                                                       bufferParam.second.descriptorInfo });
            }
        }
    }
    return buffers;
}

std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> ShaderParameters::getAllWriteTextures() const
{
    // TODO(Jeslas) : Support image view
    std::unordered_set<ImageResourceRef> uniqueness;

    std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> textures;
    for (const std::pair<const StringID, TextureParameterData> &textuteParam : shaderTextures)
    {
        for (const auto &img : textuteParam.second.textures)
        {
            if (!img.texture.isValid() || !uniqueness.emplace(img.texture).second)
                continue;

            if (img.texture->isShaderWrite()
                && BIT_SET(textuteParam.second.descriptorInfo->textureEntryPtr->data.readWriteState, EDescriptorEntryState::WriteOnly))
            {
                textures.emplace_back(std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>{ img.texture,
                                                                                                        textuteParam.second.descriptorInfo });
            }
        }
    }
    return textures;
}

std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> ShaderParameters::getAllWriteBuffers() const
{
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> buffers;
    for (const std::pair<const StringID, BufferParametersData> &bufferParam : shaderBuffers)
    {
        if ((bufferParam.second.descriptorInfo->bIsStorage
             || bufferParam.second.gpuBuffer->getType()->isChildOf(IRenderInterfaceModule::get()->currentGraphicsHelper()->writeOnlyBufferType()
             )
             || bufferParam.second.gpuBuffer->getType()->isChildOf(IRenderInterfaceModule::get()->currentGraphicsHelper()->readWriteBufferType()
             ))
            && BIT_SET(bufferParam.second.descriptorInfo->bufferEntryPtr->data.readWriteState, EDescriptorEntryState::WriteOnly))
        {
            buffers.emplace_back(std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>{ bufferParam.second.gpuBuffer,
                                                                                                   bufferParam.second.descriptorInfo });
        }
    }
    return buffers;
}

std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> ShaderParameters::getAllWriteTexels() const
{
    // TODO(Jeslas) : Support texel view
    std::unordered_set<BufferResourceRef> uniqueness;

    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> buffers;
    for (const std::pair<const StringID, TexelParameterData> &bufferParam : shaderTexels)
    {
        for (BufferResourceRef texels : bufferParam.second.gpuBuffers)
        {
            if (!texels.isValid() || !uniqueness.emplace(texels).second)
                continue;

            if ((bufferParam.second.descriptorInfo->bIsStorage
                 || texels->getType()->isChildOf(IRenderInterfaceModule::get()->currentGraphicsHelper()->writeOnlyTexelsType())
                 || texels->getType()->isChildOf(IRenderInterfaceModule::get()->currentGraphicsHelper()->readWriteTexelsType()))
                && BIT_SET(bufferParam.second.descriptorInfo->texelBufferEntryPtr->data.readWriteState, EDescriptorEntryState::WriteOnly))
            {
                buffers.emplace_back(std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>{ texels,
                                                                                                       bufferParam.second.descriptorInfo });
            }
        }
    }
    return buffers;
}

void ShaderParameters::updateParams(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance)
{
    std::vector<BatchCopyBufferData> copies;
    pullBufferParamUpdates(copies, cmdList, graphicsInstance);
    if (!copies.empty())
    {
        cmdList->copyToBuffer(copies);
    }
}

void ShaderParameters::pullBufferParamUpdates(
    std::vector<BatchCopyBufferData> &copies, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance
)
{
    std::unordered_set<BufferParameterUpdate, BufferParameterUpdate::Hasher> uniqueBufferUpdates(bufferUpdates.cbegin(), bufferUpdates.cend());

    for (const BufferParameterUpdate &bufferUpdate : uniqueBufferUpdates)
    {
        const BufferParametersData &bufferParamData = shaderBuffers.at(bufferUpdate.bufferName);
        const BufferParametersData::BufferParameter &bufferParamField = bufferParamData.bufferParams.at(bufferUpdate.paramName);

        // Offset of struct when updating field is inside inner struct,
        // In which case field offset will always be from its outer struct and we have to add this
        // offset to that for obtaining proper offset
        uint32 outerOffset = 0;
        {
            const BufferParametersData::BufferParameter *outerBufferParamField
                = bufferParamField.outerName.isValid() ? &bufferParamData.bufferParams.at(bufferParamField.outerName) : nullptr;
            while (outerBufferParamField)
            {
                if (outerBufferParamField->bufferField->isIndexAccessible())
                {
                    LOG_WARN(
                        "ShaderParameters",
                        "Setting value of parameter[%s] inside a struct[%s] in AoS[%s] will always set param value at struct index 0",
                        bufferUpdate.paramName, bufferUpdate.bufferName, outerBufferParamField->bufferField->paramName
                    );
                }

                outerOffset += outerBufferParamField->bufferField->offset;
                outerBufferParamField
                    = outerBufferParamField->outerName.isValid() ? &bufferParamData.bufferParams.at(outerBufferParamField->outerName) : nullptr;
            }
        }

        BatchCopyBufferData copyData;
        copyData.dst = bufferParamData.gpuBuffer;
        copyData.dstOffset = outerOffset + bufferParamField.bufferField->offset + (bufferUpdate.index * bufferParamField.bufferField->stride);
        copyData.dataToCopy = bufferParamField.bufferField->fieldData(bufferParamField.outerPtr, nullptr, &copyData.size);
        copyData.dataToCopy = reinterpret_cast<const uint8 *>(copyData.dataToCopy) + (bufferUpdate.index * copyData.size);

        copies.emplace_back(copyData);
    }

    ParamUpdateLambdaOut genericUpdateOut{ &copies };
    for (const ParamUpdateLambda &lambda : genericUpdates)
    {
        lambda(genericUpdateOut, cmdList, graphicsInstance);
    }
    bufferUpdates.clear();
    genericUpdates.clear();
}

std::pair<const ShaderParameters::BufferParametersData *, const ShaderParameters::BufferParametersData::BufferParameter *>
    ShaderParameters::findBufferParam(StringID &bufferName, StringID paramName) const
{
    std::pair<const BufferParametersData *, const BufferParametersData::BufferParameter *> retVal{ nullptr, nullptr };

    for (const std::pair<const StringID, BufferParametersData> &bufferParams : shaderBuffers)
    {
        auto bufferParamItr = bufferParams.second.bufferParams.find(paramName);
        if (bufferParamItr != bufferParams.second.bufferParams.cend())
        {
            bufferName = bufferParams.first;
            retVal.first = &bufferParams.second;
            retVal.second = &(*bufferParamItr).second;
            break;
        }
    }
    return retVal;
}

void *ShaderParameters::getOuterPtrForPath(
    std::vector<const BufferParametersData::BufferParameter *> &outInnerBufferParams, ArrayView<const StringID> pathNames,
    ArrayView<const uint32> indices
) const
{
    debugAssertf(indices.front() == 0, "Indexing bound buffer itself is not supported right now!");

    auto shaderBufferItr = shaderBuffers.find(pathNames.front());
    if (shaderBufferItr == shaderBuffers.cend())
    {
        LOG_ERROR("ShaderParameters", "Cannot find bounding buffer param %s!", pathNames.front());
        return nullptr;
    }
    const BufferParametersData &boundBufferData = shaderBufferItr->second;
    // -1 since 0th pathNames is just boundBufferData, And filling innerBufferParams that are more than 1 level deep from bound buffer
    outInnerBufferParams.resize(pathNames.size() - 1);
    for (uint32 i = pathNames.size() - 1; i != 1; --i)
    {
        auto itr = boundBufferData.bufferParams.find(pathNames[i]);
        if (itr != boundBufferData.bufferParams.cend() && pathNames[i - 1] == itr->second.outerName) [[likely]]
        {
            outInnerBufferParams[i - 1] = &itr->second;
        }
        else
        {
            LOG_ERROR("ShaderParameters", "Invalid pathNames[%s is not outer of %s]", pathNames[i - 1], pathNames[i]);
            return nullptr;
        }
        const BufferParametersData::BufferParameter *bufferParam = outInnerBufferParams[i - 1];

        debugAssertf(!bufferParam->bufferField->isPointer(), "Immediate inners of bound buffer can alone be a runtime resizable array");
        // If invalid index return false as well
        if (bufferParam->bufferField->isIndexAccessible() && indices[i] >= (bufferParam->bufferField->size / bufferParam->bufferField->stride))
            [[unlikely]]
        {
            LOG_ERROR(
                "ShaderParameters", "Invalid index %u max size %u for param field %s", indices[i],
                (bufferParam->bufferField->size / bufferParam->bufferField->stride), pathNames[i]
            );
            return nullptr;
        }
    }
#if DEBUG_BUILD
    fatalAssertf(
        BIT_NOT_SET(outInnerBufferParams.back()->bufferField->fieldDecorations, ShaderBufferField::IsStruct),
        "Cannot set buffer %s using setFieldAtPath", pathNames.back()
    );
    // 0th innerBufferParam will be filled after
    for (int32 i = outInnerBufferParams.size() - 2; i > 0; --i)
    {
        fatalAssertf(
            BIT_SET(outInnerBufferParams[i]->bufferField->fieldDecorations, ShaderBufferField::IsStruct),
            "Path to field is invalid![%s is not a struct]", pathNames[i + 1]
        );
    }
#endif
    // If we are here then path is valid, get subfield of boundBufferData
    outInnerBufferParams[0] = &boundBufferData.bufferParams.find(pathNames[1])->second;
    if (outInnerBufferParams[0]->bufferField->isIndexAccessible()) [[likely]]
    {
        uint32 maxCount = outInnerBufferParams[0]->bufferField->isPointer()
                              ? boundBufferData.runtimeArray->currentCount
                              : (outInnerBufferParams[0]->bufferField->size / outInnerBufferParams[0]->bufferField->stride);
        if (indices[1] >= maxCount) [[unlikely]]
        {
            LOG_ERROR("ShaderParameters", "Invalid index %u max size %u for param field %s", indices[1], maxCount, pathNames[1]);
            return nullptr;
        }
    }

    // Do not have to handle runtime parameter separately as it will be last entry and There is no support for dynamic runtime array inside one
    // Now offset the outerPtr value of param field we are looking for
    UPtrInt paramOuterPtr = (UPtrInt)(outInnerBufferParams.back()->outerPtr);
    for (uint32 i = 0; i != (outInnerBufferParams.size() - 1); ++i)
    {
        if (outInnerBufferParams[i]->bufferField->isIndexAccessible())
        {
            paramOuterPtr += indices[i + 1] * outInnerBufferParams[i]->bufferField->paramInfo->paramNativeStride();
        }
    }

    return (void *)(paramOuterPtr);
}

template <typename FieldType>
bool ShaderParameters::setFieldParam(StringID paramName, const FieldType &value, uint32 index)
{
    bool bValueSet = false;
    StringID bufferName;
    std::pair<const BufferParametersData *, const BufferParametersData::BufferParameter *> foundInfo = findBufferParam(bufferName, paramName);

    BufferParameterUpdate updateVal{ bufferName, paramName, 0 };

    if (foundInfo.first && foundInfo.second && BIT_NOT_SET(foundInfo.second->bufferField->fieldDecorations, ShaderBufferField::IsStruct))
    {
        if (foundInfo.second->bufferField->isIndexAccessible())
        {
            if (!foundInfo.second->bufferField->isPointer() || (foundInfo.first->runtimeArray->currentCount > index))
            {
                bValueSet = foundInfo.second->bufferField->setFieldDataArray(foundInfo.second->outerPtr, value, index);
                updateVal.index = index;
            }
        }
        else
        {
            bValueSet = foundInfo.second->bufferField->setFieldData(foundInfo.second->outerPtr, value);
        }
    }
    if (bValueSet)
    {
        bufferUpdates.emplace_back(updateVal);
    }
    else
    {
        LOG_ERROR(
            "ShaderParameters", "Cannot set %s[%d] of %s", paramName, index,
            bufferName.isValid() ? bufferName.toString()
                                 : TCHAR("Buffer not found")
            );
    }
    return bValueSet;
}

template <typename FieldType>
bool ShaderParameters::setFieldParam(StringID paramName, StringID bufferName, const FieldType &value, uint32 index)
{
    bool bValueSet = false;
    BufferParameterUpdate updateVal{ bufferName, paramName, 0 };

    auto bufferParamsItr = shaderBuffers.find(bufferName);
    if (bufferParamsItr != shaderBuffers.end())
    {
        auto bufferParamItr = bufferParamsItr->second.bufferParams.find(paramName);
        if (bufferParamItr != bufferParamsItr->second.bufferParams.end()
            && BIT_NOT_SET(bufferParamItr->second.bufferField->fieldDecorations, ShaderBufferField::IsStruct))
        {
            if (bufferParamItr->second.bufferField->isIndexAccessible())
            {
                if (!bufferParamItr->second.bufferField->isPointer() || (bufferParamsItr->second.runtimeArray->currentCount > index))
                {
                    bValueSet = bufferParamItr->second.bufferField->setFieldDataArray(bufferParamItr->second.outerPtr, value, index);
                    updateVal.index = index;
                }
            }
            else
            {
                bValueSet = bufferParamItr->second.bufferField->setFieldData(bufferParamItr->second.outerPtr, value);
            }
        }
    }
    if (bValueSet)
    {
        bufferUpdates.emplace_back(updateVal);
    }
    else
    {
        LOG_ERROR("ShaderParameters", "Cannot set %s[%d] of %s", paramName, index, bufferName);
    }
    return bValueSet;
}

template <typename FieldType>
bool ShaderParameters::setFieldAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const FieldType &value)
{
    // 1 or 2 can be set using setFieldParam
    if (pathNames.size() == 1)
    {
        return setFieldParam(pathNames[0], value, indices[0]);
    }
    else if (pathNames.size() == 2)
    {
        return setFieldParam(pathNames[1], pathNames[0], value, indices[1]);
    }
    else if (pathNames.size() == 0) [[unlikely]]
    {
        LOG_ERROR("ShaderParameters", "Setting field at path without valid parameters!");
        return false;
    }

    std::vector<const BufferParametersData::BufferParameter *> innerBufferParams;
    void *paramOuterPtr = getOuterPtrForPath(innerBufferParams, pathNames, indices);
    if (paramOuterPtr)
    {
        bool bValueSet = false;
        uint32 updatedIdx = 0;
        if (innerBufferParams.back()->bufferField->isIndexAccessible()) [[likely]]
        {
            updatedIdx = indices.back();
            bValueSet = innerBufferParams.back()->bufferField->setFieldDataArray(paramOuterPtr, value, updatedIdx);
        }
        else
        {
            bValueSet = innerBufferParams.back()->bufferField->setFieldData(paramOuterPtr, value);
        }

        if (bValueSet)
        {
            BufferParameterUpdate bufferUpdate = { pathNames[0], pathNames.back(), updatedIdx };
            // Mark the first non zero indexed parameter field/buffer for update, This is because only one index value can be updated
            // BufferParameterUpdate::index has to cover the entire struct that contains the edited field
            for (uint32 i = 0; i != innerBufferParams.size(); ++i)
            {
                if (indices[i + 1] != 0 && innerBufferParams[i]->bufferField->isIndexAccessible())
                {
                    bufferUpdate = { pathNames[0], pathNames[i + 1], indices[i + 1] };
                    break;
                }
            }
            bufferUpdates.emplace_back(std::move(bufferUpdate));
            return true;
        }
    }
    LOG_ERROR("ShaderParameters", "Failed to set %s parameter", pathNames.back());
    return false;
}

template <typename FieldType>
FieldType ShaderParameters::getFieldParam(StringID paramName, uint32 index) const
{
    StringID bufferName;
    std::pair<const BufferParametersData *, const BufferParametersData::BufferParameter *> foundInfo = findBufferParam(bufferName, paramName);
    // Only if accessible
    if (foundInfo.first && foundInfo.second && BIT_NOT_SET(foundInfo.second->bufferField->fieldDecorations, ShaderBufferField::IsStruct)
        && (!foundInfo.second->bufferField->isPointer() || foundInfo.first->runtimeArray->currentCount > index))
    {
        uint32 fieldTypeSize;
        const void *dataPtr = foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, &fieldTypeSize);
        if (sizeof(FieldType) == fieldTypeSize)
        {
            uint32 idx = foundInfo.second->bufferField->isIndexAccessible() ? index : 0;
            return reinterpret_cast<const FieldType *>(dataPtr)[idx];
        }
    }
    else
    {
        LOG_ERROR(
            "ShaderParameters", "Cannot get %s[%d] of %s", paramName, index,
            bufferName.isValid() ? bufferName.toString()
                                 : TCHAR("Buffer not found")
            );
    }
    return FieldType(0);
}

template <typename FieldType>
FieldType ShaderParameters::getFieldParam(StringID paramName, StringID bufferName, uint32 index) const
{
    auto bufferParamsItr = shaderBuffers.find(bufferName);
    if (bufferParamsItr != shaderBuffers.end())
    {
        auto bufferParamItr = bufferParamsItr->second.bufferParams.find(paramName);
        if (bufferParamItr != bufferParamsItr->second.bufferParams.end()
            && BIT_NOT_SET(bufferParamItr->second.bufferField->fieldDecorations, ShaderBufferField::IsStruct)
            && (!bufferParamItr->second.bufferField->isPointer() || bufferParamsItr->second.runtimeArray->currentCount > index))
        {
            uint32 fieldTypeSize;
            const void *dataPtr = bufferParamItr->second.bufferField->fieldData(bufferParamItr->second.outerPtr, nullptr, &fieldTypeSize);
            if (sizeof(FieldType) == fieldTypeSize)
            {
                uint32 idx = bufferParamItr->second.bufferField->isIndexAccessible() ? index : 0;
                return reinterpret_cast<const FieldType *>(dataPtr)[idx];
            }
        }
    }
    else
    {
        LOG_ERROR("ShaderParameters", "Cannot get %s[%d] of %s", paramName, index, bufferName);
    }
    return FieldType(0);
}

template <typename FieldType>
FieldType ShaderParameters::getFieldAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const
{
    // 1 or 2 can be set using getFieldParam
    if (pathNames.size() == 1)
    {
        return getFieldParam<FieldType>(pathNames[0], indices[0]);
    }
    else if (pathNames.size() == 2)
    {
        return getFieldParam<FieldType>(pathNames[1], pathNames[0], indices[1]);
    }
    else if (pathNames.size() == 0) [[unlikely]]
    {
        LOG_ERROR("ShaderParameters", "Getting field at path without valid parameters!");
        return FieldType(0);
    }

    std::vector<const BufferParametersData::BufferParameter *> innerBufferParams;
    const void *paramOuterPtr = getOuterPtrForPath(innerBufferParams, pathNames, indices);
    if (paramOuterPtr)
    {
        uint32 fieldTypeSize;
        const void *dataPtr = innerBufferParams.back()->bufferField->fieldData(paramOuterPtr, nullptr, &fieldTypeSize);
        if (sizeof(FieldType) == fieldTypeSize)
        {
            uint32 idx = innerBufferParams.back()->bufferField->isIndexAccessible() ? indices.back() : 0;
            return reinterpret_cast<const FieldType *>(dataPtr)[idx];
        }
    }
    return FieldType(0);
}

bool ShaderParameters::setIntParam(StringID paramName, StringID bufferName, int32 value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setIntParam(StringID paramName, StringID bufferName, uint32 value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setIntParam(StringID paramName, int32 value, uint32 index /* = 0 */) { return setFieldParam(paramName, value, index); }

bool ShaderParameters::setIntParam(StringID paramName, uint32 value, uint32 index /* = 0 */) { return setFieldParam(paramName, value, index); }

bool ShaderParameters::setFloatParam(StringID paramName, StringID bufferName, float value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setFloatParam(StringID paramName, float value, uint32 index /* = 0 */) { return setFieldParam(paramName, value, index); }

bool ShaderParameters::setVector2Param(StringID paramName, StringID bufferName, const Vector2D &value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setVector2Param(StringID paramName, const Vector2D &value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setVector4Param(StringID paramName, StringID bufferName, const Vector4D &value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setVector4Param(StringID paramName, const Vector4D &value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setMatrixParam(StringID paramName, StringID bufferName, const Matrix4 &value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setMatrixParam(StringID paramName, const Matrix4 &value, uint32 index /* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setIntAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, int32 value)
{
    return setFieldAtPath(pathNames, indices, value);
}

bool ShaderParameters::setIntAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, uint32 value)
{
    return setFieldAtPath(pathNames, indices, value);
}

bool ShaderParameters::setFloatAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, float value)
{
    return setFieldAtPath(pathNames, indices, value);
}

bool ShaderParameters::setVector2AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const Vector2D &value)
{
    return setFieldAtPath(pathNames, indices, value);
}

bool ShaderParameters::setVector4AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const Vector4D &value)
{
    return setFieldAtPath(pathNames, indices, value);
}

bool ShaderParameters::setMatrixAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const Matrix4 &value)
{
    return setFieldAtPath(pathNames, indices, value);
}

bool ShaderParameters::setTexelParam(StringID paramName, BufferResourceRef texelBuffer, uint32 index /*= 0*/)
{
    auto texelParamItr = shaderTexels.find(paramName);
    if (texelParamItr != shaderTexels.end() && texelParamItr->second.gpuBuffers.size() > index)
    {
        if (texelParamItr->second.gpuBuffers[index] != texelBuffer)
        {
            texelParamItr->second.gpuBuffers[index] = texelBuffer;
            texelUpdates.insert({ texelParamItr->first, index });
        }
        return true;
    }
    return false;
}

bool ShaderParameters::setTextureParam(StringID paramName, ImageResourceRef texture, SamplerRef sampler, uint32 index /* = 0 */)
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.end() && textureParamItr->second.textures.size() > index)
    {
        textureParamItr->second.textures[index].texture = texture;
        textureParamItr->second.textures[index].sampler = sampler;
        textureUpdates.insert({ textureParamItr->first, index });
        return true;
    }
    return false;
}

bool ShaderParameters::setTextureParam(StringID paramName, ImageResourceRef texture, uint32 index /* = 0 */)
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.end() && textureParamItr->second.textures.size() > index)
    {
        textureParamItr->second.textures[index].texture = texture;
        textureUpdates.insert({ textureParamItr->first, index });
        return true;
    }
    return false;
}

bool ShaderParameters::setTextureParamViewInfo(StringID paramName, const ImageViewInfo &textureViewInfo, uint32 index /*= 0*/)
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.end() && textureParamItr->second.textures.size() > index)
    {
        textureParamItr->second.textures[index].viewInfo = textureViewInfo;
        textureUpdates.insert({ textureParamItr->first, index });
        return true;
    }
    return false;
}

bool ShaderParameters::setSamplerParam(StringID paramName, SamplerRef sampler, uint32 index /*= 0*/)
{
    auto samplerParamItr = shaderSamplers.find(paramName);
    if (samplerParamItr != shaderSamplers.end() && samplerParamItr->second.samplers.size() > index)
    {
        samplerParamItr->second.samplers[index] = sampler;
        samplerUpdates.insert({ samplerParamItr->first, index });
        return true;
    }
    return false;
}

int32 ShaderParameters::getIntParam(StringID paramName, StringID bufferName, uint32 index /* = 0 */) const
{
    return getFieldParam<int32>(paramName, bufferName, index);
}

int32 ShaderParameters::getIntParam(StringID paramName, uint32 index /* = 0 */) const { return getFieldParam<int32>(paramName, index); }

uint32 ShaderParameters::getUintParam(StringID paramName, StringID bufferName, uint32 index /* = 0 */) const
{
    return getFieldParam<uint32>(paramName, bufferName, index);
}

uint32 ShaderParameters::getUintParam(StringID paramName, uint32 index /* = 0 */) const { return getFieldParam<uint32>(paramName, index); }

float ShaderParameters::getFloatParam(StringID paramName, StringID bufferName, uint32 index /* = 0 */) const
{
    return getFieldParam<float>(paramName, bufferName, index);
}

float ShaderParameters::getFloatParam(StringID paramName, uint32 index /* = 0 */) const { return getFieldParam<float>(paramName, index); }

Vector2D ShaderParameters::getVector2Param(StringID paramName, StringID bufferName, uint32 index /* = 0 */) const
{
    return getFieldParam<Vector2D>(paramName, bufferName, index);
}

Vector2D ShaderParameters::getVector2Param(StringID paramName, uint32 index /* = 0 */) const
{
    return getFieldParam<Vector2D>(paramName, index);
}

Vector4D ShaderParameters::getVector4Param(StringID paramName, StringID bufferName, uint32 index /* = 0 */) const
{
    return getFieldParam<Vector4D>(paramName, bufferName, index);
}

Vector4D ShaderParameters::getVector4Param(StringID paramName, uint32 index /* = 0 */) const
{
    return getFieldParam<Vector4D>(paramName, index);
}

Matrix4 ShaderParameters::getMatrixParam(StringID paramName, uint32 index /* = 0 */) const { return getFieldParam<Matrix4>(paramName, index); }

Matrix4 ShaderParameters::getMatrixParam(StringID paramName, StringID bufferName, uint32 index /* = 0 */) const
{
    return getFieldParam<Matrix4>(paramName, index);
}

int32 ShaderParameters::getIntAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const
{
    return getFieldAtPath<int32>(pathNames, indices);
}

uint32 ShaderParameters::getUintAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const
{
    return getFieldAtPath<int32>(pathNames, indices);
}

float ShaderParameters::getFloatAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const
{
    return getFieldAtPath<int32>(pathNames, indices);
}

Vector2D ShaderParameters::getVector2AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const
{
    return getFieldAtPath<Vector2D>(pathNames, indices);
}

Vector4D ShaderParameters::getVector4AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const
{
    return getFieldAtPath<Vector4D>(pathNames, indices);
}

Matrix4 ShaderParameters::getMatrixAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const
{
    return getFieldAtPath<Matrix4>(pathNames, indices);
}

BufferResourceRef ShaderParameters::getTexelParam(StringID paramName, uint32 index /*= 0*/) const
{
    auto texelParamItr = shaderTexels.find(paramName);
    if (texelParamItr != shaderTexels.end() && texelParamItr->second.gpuBuffers.size() > index)
    {
        return texelParamItr->second.gpuBuffers[index];
    }
    return nullptr;
}

ImageResourceRef ShaderParameters::getTextureParam(StringID paramName, uint32 index /* = 0 */) const
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.cend() && textureParamItr->second.textures.size() > index)
    {
        return textureParamItr->second.textures[index].texture;
    }
    return nullptr;
}

ImageResourceRef ShaderParameters::getTextureParam(SamplerRef &outSampler, StringID paramName, uint32 index /* = 0 */) const
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.cend() && textureParamItr->second.textures.size() > index)
    {
        outSampler = textureParamItr->second.textures[index].sampler;
        return textureParamItr->second.textures[index].texture;
    }
    outSampler = nullptr;
    return nullptr;
}

SamplerRef ShaderParameters::getSamplerParam(StringID paramName, uint32 index /*= 0*/) const
{
    auto samplerParamItr = shaderSamplers.find(paramName);
    if (samplerParamItr != shaderSamplers.cend() && samplerParamItr->second.samplers.size() > index)
    {
        return samplerParamItr->second.samplers[index];
    }
    return nullptr;
}

bool ShaderParameters::setBufferResource(StringID bufferName, BufferResourceRef buffer)
{
    auto bufferDataItr = shaderBuffers.find(bufferName);
    if (bufferDataItr != shaderBuffers.end())
    {
        BufferParametersData &bufferData = bufferDataItr->second;
        bufferData.bIsExternal = true;
        bufferData.gpuBuffer = buffer;
        if (bufferData.runtimeArray.has_value())
        {
            auto paramFieldItr = bufferData.bufferParams.find(bufferData.runtimeArray->paramName);
            BufferParametersData::BufferParameter &runtimeFieldParam = paramFieldItr->second;
            const ShaderBufferField *runtimeField = runtimeFieldParam.bufferField;

            const uint64 gpuByteSize = buffer->getResourceSize() - bufferData.runtimeArray->offset;
            // NOTE(Jeslas) : If this stride logic changes here. Change it in resizeRuntimeBuffer()
            const uint32 gpuDataStride = BIT_SET(runtimeField->fieldDecorations, ShaderBufferField::IsStruct)
                                             ? runtimeField->paramInfo->paramStride()
                                             : runtimeField->stride;
            const uint32 dataStride = BIT_SET(runtimeField->fieldDecorations, ShaderBufferField::IsStruct)
                                          ? runtimeField->paramInfo->paramNativeStride()
                                          : runtimeField->stride;
            bufferData.runtimeArray->currentCount = gpuByteSize / gpuDataStride;
            bufferData.runtimeArray->runtimeArrayCpuBuffer.resize(bufferData.runtimeArray->currentCount * dataStride);

            // Set buffer ptr and regenerate buffer param maps
            (*reinterpret_cast<void **>(runtimeField->fieldPtr(runtimeFieldParam.outerPtr)))
                = bufferData.runtimeArray->runtimeArrayCpuBuffer.data();
            bufferData.bufferParams.clear();
            initBufferParams(
                bufferData, bufferData.descriptorInfo->bufferParamInfo, bufferData.cpuBuffer, StringID(EInitType::InitType_NoInit)
            );
        }
        bufferResourceUpdates.insert(bufferName);
    }
    return false;
}

BufferResourceRef ShaderParameters::getBufferResource(StringID bufferName)
{
    auto bufferDataItr = shaderBuffers.find(bufferName);
    return (bufferDataItr != shaderBuffers.end()) ? bufferDataItr->second.gpuBuffer : nullptr;
}

uint32 ShaderParameters::getBufferRequiredSize(StringID bufferName) const
{
    auto bufferDataItr = shaderBuffers.find(bufferName);
    if (bufferDataItr != shaderBuffers.end())
    {
        const BufferParametersData &bufferData = bufferDataItr->second;
        debugAssertf(
            !bufferData.runtimeArray.has_value(),
            "Runtime array requires count to calculated required buffer size! %s function will return assuming runtime array count as 1",
            __func__
        );
        if (bufferData.runtimeArray.has_value())
        {
            return getRuntimeBufferRequiredSize(bufferName, 1);
        }

        return bufferData.descriptorInfo->bufferParamInfo->paramStride();
    }
    return 0;
}

uint32 ShaderParameters::getRuntimeBufferRequiredSize(StringID bufferName, uint32 count) const
{
    auto bufferDataItr = shaderBuffers.find(bufferName);
    if (bufferDataItr != shaderBuffers.end())
    {
        const BufferParametersData &bufferData = bufferDataItr->second;
        debugAssert(bufferData.runtimeArray.has_value());

        const BufferParametersData::BufferParameter &runtimeFieldParam
            = bufferData.bufferParams.find(bufferData.runtimeArray->paramName)->second;
        const ShaderBufferField *runtimeField = runtimeFieldParam.bufferField;

        // NOTE(Jeslas) : If this stride logic changes here. Change it in resizeRuntimeBuffer()
        const uint32 gpuDataStride = BIT_SET(runtimeField->fieldDecorations, ShaderBufferField::IsStruct)
                                         ? runtimeField->paramInfo->paramStride()
                                         : runtimeField->stride;

        return bufferData.runtimeArray->offset + gpuDataStride * count;
    }
    return 0;
}

uint32 ShaderParameters::getRuntimeBufferGpuStride(StringID bufferName) const
{
    auto bufferDataItr = shaderBuffers.find(bufferName);
    if (bufferDataItr != shaderBuffers.end())
    {
        const BufferParametersData &bufferData = bufferDataItr->second;
        debugAssert(bufferData.runtimeArray.has_value());

        const BufferParametersData::BufferParameter &runtimeFieldParam
            = bufferData.bufferParams.find(bufferData.runtimeArray->paramName)->second;
        const ShaderBufferField *runtimeField = runtimeFieldParam.bufferField;

        // NOTE(Jeslas) : If this stride logic changes here. Change it in resizeRuntimeBuffer()
        const uint32 gpuDataStride = BIT_SET(runtimeField->fieldDecorations, ShaderBufferField::IsStruct)
                                         ? runtimeField->paramInfo->paramStride()
                                         : runtimeField->stride;
        return gpuDataStride;
    }
    return 0;
}

uint32 ShaderParameters::getRuntimeBufferCount(StringID bufferName) const
{
    auto bufferDataItr = shaderBuffers.find(bufferName);
    if (bufferDataItr != shaderBuffers.end())
    {
        const BufferParametersData &bufferData = bufferDataItr->second;
        debugAssert(bufferData.runtimeArray.has_value());
        return bufferData.runtimeArray->currentCount;
    }
    return 0;
}

void ShaderParameters::resizeRuntimeBuffer(StringID bufferName, uint32 minCount)
{
    auto bufferDataItr = shaderBuffers.find(bufferName);
    if (bufferDataItr == shaderBuffers.end())
    {
        LOG_ERROR("ShaderParameters", "Buffer %s not found", bufferName);
        return;
    }

    BufferParametersData &bufferData = bufferDataItr->second;
    String bufferNameString = UTF8_TO_TCHAR(
        bufferData.descriptorInfo->bufferEntryPtr ? bufferData.descriptorInfo->bufferEntryPtr->attributeName.c_str()
                                                  : bufferData.descriptorInfo->texelBufferEntryPtr->attributeName.c_str()
    );
    if (bufferDataItr->second.bIsExternal)
    {
        LOG_ERROR("ShaderParameters", "External buffer assigned to %s cannot be resized", bufferNameString);
        return;
    }

    if (bufferData.runtimeArray.has_value())
    {
        auto paramFieldItr = bufferData.bufferParams.find(bufferData.runtimeArray->paramName);
        BufferParametersData::BufferParameter &runtimeFieldParam = paramFieldItr->second;
        const ShaderBufferField *runtimeField = runtimeFieldParam.bufferField;

        if (bufferData.runtimeArray->currentCount < minCount)
        {
            // NOTE(Jeslas) : If this stride logic changes here. Change it in setBufferResource()
            const uint32 dataStride = BIT_SET(runtimeField->fieldDecorations, ShaderBufferField::IsStruct)
                                          ? runtimeField->paramInfo->paramNativeStride()
                                          : runtimeField->stride;
            const uint32 newArraySize = Math::toHigherPowOf2(minCount * dataStride);
            bufferData.runtimeArray->runtimeArrayCpuBuffer.resize(newArraySize);
            bufferData.runtimeArray->currentCount = uint32(Math::floor(newArraySize / float(dataStride)));

            // Set buffer ptr and regenerate buffer param maps
            (*reinterpret_cast<void **>(runtimeField->fieldPtr(runtimeFieldParam.outerPtr)))
                = bufferData.runtimeArray->runtimeArrayCpuBuffer.data();
            bufferData.bufferParams.clear();
            initBufferParams(
                bufferData, bufferData.descriptorInfo->bufferParamInfo, bufferData.cpuBuffer, StringID(EInitType::InitType_NoInit)
            );

            ENQUEUE_COMMAND(ResizeRuntimeBuffer)
            (
                [this, bufferName, bufferNameString, runtimeField,
                 &bufferData](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
                {
                    BufferResourceRef oldBuffer = bufferData.gpuBuffer;
                    // NOTE(Jeslas) : If this stride logic changes here. Change it in setBufferResource()
                    const uint32 gpuDataStride = BIT_SET(runtimeField->fieldDecorations, ShaderBufferField::IsStruct)
                                                     ? runtimeField->paramInfo->paramStride()
                                                     : runtimeField->stride;

                    // Since only storage can be runtime array
                    bufferData.gpuBuffer = graphicsHelper->createWriteOnlyBuffer(
                        graphicsInstance, bufferData.runtimeArray->offset + bufferData.runtimeArray->currentCount * gpuDataStride
                    );
                    bufferData.gpuBuffer->setResourceName(
                        bufferNameString + TCHAR("_") + runtimeField->paramName.toString() + TCHAR("_RuntimeSoA"));
                    bufferData.gpuBuffer->init();

                    // Push descriptor update
                    bufferResourceUpdates.insert(bufferName);

                    fatalAssertf(bufferData.gpuBuffer->isValid(), "Runtime array initialization failed");
                    if (oldBuffer.isValid())
                    {
                        if (oldBuffer->isValid())
                        {
                            CopyBufferInfo copyRange{ 0, 0, uint32(oldBuffer->getResourceSize()) };
                            cmdList->copyBuffer(oldBuffer, bufferData.gpuBuffer, { &copyRange, 1 });
                        }
                    }
                }
            );
        }
    }
}
