#include "ShaderParameterResources.h"
#include "ShaderReflected.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../ShaderCore/ShaderParameterUtility.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "../../Core/Math/Vector2D.h"
#include "../../Core/Math/Vector4D.h"
#include "../../Core/Math/Matrix4.h"

#include <unordered_set>

void ShaderDescriptorParamType::wrapReflectedDescriptors(std::map<String, ShaderDescriptorParamType*>& descriptorParams
    , const ReflectDescriptorBody& reflectDescriptors, std::map<String, ShaderBufferDescriptorType*>* filterBufferDescriptors /*= nullptr*/)
{
    for (const DescEntryBuffer& descriptorInfo : reflectDescriptors.uniforms)
    {
        ShaderBufferDescriptorType* bufferDescriptorWrapper = new ShaderBufferDescriptorType();
        bufferDescriptorWrapper->bIsStorage = false;
        bufferDescriptorWrapper->bufferEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = bufferDescriptorWrapper;
        if (filterBufferDescriptors)
        {
            (*filterBufferDescriptors)[descriptorInfo.attributeName] = bufferDescriptorWrapper;
        }
    }
    for (const DescEntryBuffer& descriptorInfo : reflectDescriptors.buffers)
    {
        ShaderBufferDescriptorType* bufferDescriptorWrapper = new ShaderBufferDescriptorType();
        bufferDescriptorWrapper->bIsStorage = true;
        bufferDescriptorWrapper->bufferEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = bufferDescriptorWrapper;
        if (filterBufferDescriptors)
        {
            (*filterBufferDescriptors)[descriptorInfo.attributeName] = bufferDescriptorWrapper;
        }
    }
    for (const DescEntryTexelBuffer& descriptorInfo : reflectDescriptors.imageBuffers)
    {
        ShaderBufferDescriptorType* texelDescriptorWrapper = new ShaderBufferDescriptorType();
        texelDescriptorWrapper->bIsStorage = true;
        texelDescriptorWrapper->texelBufferEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = texelDescriptorWrapper;
    }
    for (const DescEntryTexelBuffer& descriptorInfo : reflectDescriptors.samplerBuffers)
    {
        ShaderBufferDescriptorType* texelDescriptorWrapper = new ShaderBufferDescriptorType();
        texelDescriptorWrapper->bIsStorage = false;
        texelDescriptorWrapper->texelBufferEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = texelDescriptorWrapper;
    }
    for (const DescEntryTexture& descriptorInfo : reflectDescriptors.imagesAndImgArrays)
    {
        ShaderTextureDescriptorType* textureDescriptorWrapper = new ShaderTextureDescriptorType();
        textureDescriptorWrapper->bIsAttachedSampler = false;
        textureDescriptorWrapper->imageUsageFlags = EImageShaderUsage::Writing;
        textureDescriptorWrapper->textureEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = textureDescriptorWrapper;
    }
    for (const DescEntryTexture& descriptorInfo : reflectDescriptors.textureAndArrays)
    {
        ShaderTextureDescriptorType* textureDescriptorWrapper = new ShaderTextureDescriptorType();
        textureDescriptorWrapper->bIsAttachedSampler = false;
        textureDescriptorWrapper->imageUsageFlags = EImageShaderUsage::Sampling;
        textureDescriptorWrapper->textureEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = textureDescriptorWrapper;
    }
    for (const DescEntryTexture& descriptorInfo : reflectDescriptors.sampledTexAndArrays)
    {
        ShaderTextureDescriptorType* textureDescriptorWrapper = new ShaderTextureDescriptorType();
        textureDescriptorWrapper->bIsAttachedSampler = true;
        textureDescriptorWrapper->imageUsageFlags = EImageShaderUsage::Sampling;
        textureDescriptorWrapper->textureEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = textureDescriptorWrapper;
    }
    for (const DescEntrySampler& descriptorInfo : reflectDescriptors.samplers)
    {
        ShaderSamplerDescriptorType* samplerDescriptorWrapper = new ShaderSamplerDescriptorType();
        samplerDescriptorWrapper->samplerEntryPtr = &descriptorInfo;
        descriptorParams[descriptorInfo.attributeName] = samplerDescriptorWrapper;
    }
    for (const DescEntrySubpassInput& descriptorInfo : reflectDescriptors.subpassInputs)
    {
        Logger::warn("DescriptorTypeParams", "%s() : Sub pass inputs are not supported yet %s", __func__, descriptorInfo.attributeName.c_str());
    }
}

template<typename DescriptorParamType>
const DescriptorParamType* Cast(const ShaderDescriptorParamType* shaderDescriptorType)
{
    if (DescriptorParamType::PARAM_TYPE == shaderDescriptorType->paramType)
    {
        return static_cast<const DescriptorParamType*>(shaderDescriptorType);
    }
    return nullptr;
}

template<typename DescriptorParamType>
DescriptorParamType* Cast(ShaderDescriptorParamType* shaderDescriptorType)
{
    if (DescriptorParamType::PARAM_TYPE == shaderDescriptorType->paramType)
    {
        return static_cast<DescriptorParamType*>(shaderDescriptorType);
    }
    return nullptr;
}

ShaderTextureDescriptorType::ShaderTextureDescriptorType()
{
    paramType = ShaderDescriptorParamType::Texture;
}
ShaderBufferDescriptorType::ShaderBufferDescriptorType()
{
    paramType = ShaderDescriptorParamType::Buffer;
}
ShaderSamplerDescriptorType::ShaderSamplerDescriptorType()
{
    paramType = ShaderDescriptorParamType::Sampler;
}

//////////////////////////////////////////////////////////////////////////
// ShaderSetParametersLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderSetParametersLayout)

ShaderSetParametersLayout::ShaderSetParametersLayout(const ShaderResource* shaderResource, uint32 setID)
    : respectiveShaderRes(shaderResource)
    , shaderSetID(setID)
{}

void ShaderSetParametersLayout::init()
{
    BaseType::init();
    const ShaderReflected* shaderReflection = respectiveShaderRes->getReflection();

    std::map<String, ShaderBufferDescriptorType*> bufferDescriptors;
    for (const ReflectDescriptorBody& descriptorsSet : shaderReflection->descriptorsSets)
    {
        if (descriptorsSet.set == shaderSetID)
        {
            ShaderDescriptorParamType::wrapReflectedDescriptors(paramsLayout, descriptorsSet, &bufferDescriptors);
        }
    }

    bindBufferParamInfo(bufferDescriptors);
    std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
    {
        std::map<String, SpecializationConstantEntry> specConsts;
        respectiveShaderRes->getSpecializationConsts(specConsts);
        ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderReflection);
    }

    // Fill those bound buffer info with GPU reflect data
    for (const std::pair<const String, ShaderBufferDescriptorType*>& bufferDescWrapper : bufferDescriptors)
    {
        bufferDescWrapper.second->bufferNativeStride = bufferDescWrapper.second->bufferParamInfo->paramStride();
        ShaderParameterUtility::fillRefToBufParamInfo(*bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data, specializationConsts);
    }
}

void ShaderSetParametersLayout::release()
{
    for (const std::pair<const String, ShaderDescriptorParamType*>& shaderDescriptorTypeWrapper : paramsLayout)
    {
        delete shaderDescriptorTypeWrapper.second;
    }
    paramsLayout.clear();

    BaseType::release();
}

const ShaderDescriptorParamType* ShaderSetParametersLayout::parameterDescription(const String& paramName) const
{
    uint32 temp;
    return parameterDescription(temp, paramName);
}

const ShaderDescriptorParamType* ShaderSetParametersLayout::parameterDescription(uint32& outSetIdx, const String& paramName) const
{
    auto foundParamItr = paramsLayout.find(paramName);
    if (foundParamItr != paramsLayout.cend())
    {
        outSetIdx = shaderSetID;
        return foundParamItr->second;
    }
    Logger::error("ShaderSetParametersLayout", "%s() : Parameter %s is not available in shader %s at set %u", __func__
        , paramName.getChar(), respectiveShaderRes->getResourceName().getChar(), shaderSetID);
    return nullptr;
}

const std::map<String, ShaderDescriptorParamType*>& ShaderSetParametersLayout::allParameterDescriptions() const
{
    return paramsLayout;
}

//////////////////////////////////////////////////////////////////////////
// ShaderParametersLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderParametersLayout)

ShaderParametersLayout::ShaderParametersLayout(const ShaderResource* shaderResource)
    : respectiveShaderRes(shaderResource)
{}

void ShaderParametersLayout::init()
{
    BaseType::init();

    const ShaderReflected* shaderReflection = respectiveShaderRes->getReflection();

    std::map<uint32, std::map<String, ShaderDescriptorParamType*>> setToParamsLayout;

    // Wrapping descriptors sets reflected info into ShaderDescriptorParamType wrappers
    std::map<String, ShaderBufferDescriptorType*> bufferDescriptors;
    for (const ReflectDescriptorBody& descriptorsSet : shaderReflection->descriptorsSets)
    {
        ShaderDescriptorParamType::wrapReflectedDescriptors(setToParamsLayout[descriptorsSet.set], descriptorsSet, &bufferDescriptors);
    }

    // Fill those bound buffer info with GPU reflect data
    respectiveShaderRes->bindBufferParamInfo(bufferDescriptors);
    std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
    {
        std::map<String, SpecializationConstantEntry> specConsts;
        respectiveShaderRes->getSpecializationConsts(specConsts);
        ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderReflection);
    }

    for (const std::pair<const String, ShaderBufferDescriptorType*>& bufferDescWrapper : bufferDescriptors)
    {
        bufferDescWrapper.second->bufferNativeStride = bufferDescWrapper.second->bufferParamInfo->paramStride();
        ShaderParameterUtility::fillRefToBufParamInfo(*bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data, specializationConsts);
    }

    for (const std::pair<const uint32, std::map<String, ShaderDescriptorParamType*>>& setToDescriptorsPair : setToParamsLayout)
    {
        for (const std::pair<const String, ShaderDescriptorParamType*>& descriptorWrapper : setToDescriptorsPair.second)
        {
            // Since currently we support only one unique name per shader
            fatalAssert(paramsLayout.find(descriptorWrapper.first) == paramsLayout.end(), "Shader descriptor param name must be unique for a shader pipeline");
            paramsLayout.insert({ descriptorWrapper.first, { setToDescriptorsPair.first, descriptorWrapper.second } });
        }
    }
}

void ShaderParametersLayout::release()
{
    for (const std::pair<const String, std::pair<uint32, ShaderDescriptorParamType*>>& shaderDescriptorTypeWrapper : paramsLayout)
    {
        delete shaderDescriptorTypeWrapper.second.second;
    }
    paramsLayout.clear();

    BaseType::release();
}

const ShaderDescriptorParamType* ShaderParametersLayout::parameterDescription(uint32& outSetIdx, const String& paramName) const
{
    auto foundParamItr = paramsLayout.find(paramName);
    if (foundParamItr != paramsLayout.cend())
    {
        outSetIdx = foundParamItr->second.first;
        return foundParamItr->second.second;
    }
    Logger::error("ShaderParametersLayout", "%s() : Parameter %s is not available in shader %s", __func__
        , paramName.getChar(), respectiveShaderRes->getResourceName().getChar());
    return nullptr;
}

const ShaderDescriptorParamType* ShaderParametersLayout::parameterDescription(const String& paramName) const
{
    uint32 temp;
    return parameterDescription(temp, paramName);
}


std::map<String, ShaderDescriptorParamType*> ShaderParametersLayout::allParameterDescriptions() const
{
    std::map<String, ShaderDescriptorParamType*> allParamsLayout;
    for (const std::pair<const String, std::pair<uint32, ShaderDescriptorParamType*>>& paramLayout : paramsLayout)
    {
        allParamsLayout[paramLayout.first] = paramLayout.second.second;
    }
    return allParamsLayout;
}

uint32 ShaderParametersLayout::getSetID(const String& paramName) const
{
    auto foundParamItr = paramsLayout.find(paramName);
    fatalAssert(foundParamItr != paramsLayout.cend(), "Cannot call this function with invalid param name, Use parameterDescription function if validity is not sure");
    return foundParamItr->second.first;
}

//////////////////////////////////////////////////////////////////////////
// ShaderParameters
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderParameters)

size_t ShaderParameters::BufferParameterUpdate::Hasher::operator()(const BufferParameterUpdate& keyVal) const noexcept
{
    size_t seed = HashUtility::hash(keyVal.bufferName);
    HashUtility::hashCombine(seed, keyVal.paramName);
    HashUtility::hashCombine(seed, keyVal.index);
    return seed;
}

ShaderParameters::ShaderParameters(const GraphicsResource* shaderParamLayout, const std::set<uint32>& ignoredSetIds/* = {}*/)
    : GraphicsResource()
    , paramLayout(shaderParamLayout)
    , ignoredSets(ignoredSetIds)
{
    if (paramLayout->getType()->isChildOf<ShaderSetParametersLayout>())
    {
        std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
        {
            const ShaderResource* shaderRes = static_cast<const ShaderSetParametersLayout*>(paramLayout)->getShaderResource();
            std::map<String, SpecializationConstantEntry> specConsts;
            shaderRes->getSpecializationConsts(specConsts);
            ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderRes->getReflection());
        }
        initParamsMaps(static_cast<const ShaderSetParametersLayout*>(paramLayout)->allParameterDescriptions(), specializationConsts);
    }
    else if (paramLayout->getType()->isChildOf<ShaderParametersLayout>())
    {
        std::map<String, ShaderDescriptorParamType*> allParameters = static_cast<const ShaderParametersLayout*>(paramLayout)->allParameterDescriptions();
        if (!ignoredSets.empty())
        {
            for (auto itr = allParameters.begin(); itr != allParameters.end();)
            {
                if (ignoredSets.find(static_cast<const ShaderParametersLayout*>(paramLayout)->getSetID(itr->first)) == ignoredSets.end())
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
            const ShaderResource* shaderRes = static_cast<const ShaderParametersLayout*>(paramLayout)->getShaderResource();
            std::map<String, SpecializationConstantEntry> specConsts;
            shaderRes->getSpecializationConsts(specConsts);
            ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, shaderRes->getReflection());
        }
        initParamsMaps(allParameters, specializationConsts);
    }
    else
    {
        fatalAssert(false, "Unsupported Shader parameters layout");
    }
}

void ShaderParameters::init()
{
    BaseType::init();
    for (const std::pair<const String, BufferParametersData>& bufferParameters : shaderBuffers)
    {
        // Only if not using already set resource externally
        if(!bufferParameters.second.gpuBuffer->isValid())
        {
            bufferParameters.second.gpuBuffer->setResourceName(bufferParameters.first);
            bufferParameters.second.gpuBuffer->init();
        }
    }
}

void ShaderParameters::initBufferParams(BufferParametersData& bufferParamData, const ShaderBufferParamInfo* bufferParamInfo, void* outerPtr, const char* outerName) const
{
    const ShaderBufferFieldNode* currentNode = &bufferParamInfo->startNode;
    while (currentNode->isValid())
    {
        bufferParamData.bufferParams[currentNode->field->paramName] = { outerPtr, (outerName)? outerName : "", currentNode->field};
        if (currentNode->field->bIsStruct)
        {
            // AoS inside shader base uniform struct is supported, AoSoA... not supported due to parameter indexing limitation being 1 right now
            if (outerName != nullptr && currentNode->field->bIsArray)
            {
                fatalAssert(!"We do not support nested array in parameters", "We do not support nested array in parameters");
            }
            void* nextOuterPtr = nullptr;
            {
                nextOuterPtr = currentNode->field->fieldData(outerPtr, nullptr, nullptr);
            }
            initBufferParams(bufferParamData, currentNode->field->paramInfo, nextOuterPtr, currentNode->field->paramName.getChar());
        }
        currentNode = currentNode->nextNode;
    }
}

void ShaderParameters::initParamsMaps(const std::map<String, ShaderDescriptorParamType*>& paramsDesc, const std::vector<std::vector<SpecializationConstantEntry>>& specializationConsts)
{
    for (const std::pair<const String, ShaderDescriptorParamType*>& paramDesc : paramsDesc)
    {
        if (const ShaderBufferDescriptorType* bufferParamDesc = Cast<ShaderBufferDescriptorType>(paramDesc.second))
        {
            if (bufferParamDesc->bufferEntryPtr != nullptr)
            {
                BufferParametersData paramData;
                paramData.descriptorInfo = bufferParamDesc;
                paramData.cpuBuffer = new uint8[bufferParamDesc->bufferNativeStride];
                memset(paramData.cpuBuffer, 0, bufferParamDesc->bufferNativeStride);
                if(bufferParamDesc->bIsStorage)
                {
                    paramData.gpuBuffer = new GraphicsWBuffer(bufferParamDesc->bufferParamInfo->paramStride());
                }
                else 
                {
                    paramData.gpuBuffer = new GraphicsRBuffer(bufferParamDesc->bufferParamInfo->paramStride());
                }

                initBufferParams(paramData, bufferParamDesc->bufferParamInfo, paramData.cpuBuffer, nullptr);
                shaderBuffers[bufferParamDesc->bufferEntryPtr->attributeName] = paramData;
            }
            else
            {
                debugAssert(bufferParamDesc->texelBufferEntryPtr->data.data.arraySize.size() == 1);
                uint32 count = 0;
                const ArrayDefinition& arrayDef = bufferParamDesc->texelBufferEntryPtr->data.data.arraySize[0];
                if (!arrayDef.isSpecializationConst)
                {
                    count = arrayDef.dimension;
                }
                else if (!SpecializationConstUtility::asValue(count, specializationConsts[arrayDef.stageIdx][arrayDef.dimension]))
                {
                    Logger::error("ShaderParameters", "%s() : Texel buffer %s array count specialization invalid"
                        , __func__, bufferParamDesc->texelBufferEntryPtr->attributeName.c_str());
                }

                TexelParameterData& paramData = shaderTexels[bufferParamDesc->texelBufferEntryPtr->attributeName];
                paramData.descriptorInfo = bufferParamDesc;
                paramData.gpuBuffers.resize(count, nullptr);
            }
        }
        else if (const ShaderTextureDescriptorType* textureParamDesc = Cast<ShaderTextureDescriptorType>(paramDesc.second))
        {
            debugAssert(textureParamDesc->textureEntryPtr->data.data.arraySize.size() == 1);
            uint32 count = 0;
            const ArrayDefinition& arrayDef = textureParamDesc->textureEntryPtr->data.data.arraySize[0];
            if (!arrayDef.isSpecializationConst)
            {
                count = arrayDef.dimension;
            }
            else if (!SpecializationConstUtility::asValue(count, specializationConsts[arrayDef.stageIdx][arrayDef.dimension]))
            {
                Logger::error("ShaderParameters", "%s() : Texture %s array count specialization invalid"
                    , __func__, textureParamDesc->textureEntryPtr->attributeName.c_str());
            }

            TextureParameterData& paramData = shaderTextures[textureParamDesc->textureEntryPtr->attributeName];
            paramData.textures.resize(count);
            paramData.descriptorInfo = textureParamDesc;
        }
        else if (const ShaderSamplerDescriptorType* samplerParamDesc = Cast<ShaderSamplerDescriptorType>(paramDesc.second))
        {
            debugAssert(samplerParamDesc->samplerEntryPtr->data.data.size() == 1);
            uint32 count = 0;
            const ArrayDefinition& arrayDef = samplerParamDesc->samplerEntryPtr->data.data[0];
            if (!arrayDef.isSpecializationConst)
            {
                count = arrayDef.dimension;
            }
            else if (!SpecializationConstUtility::asValue(count, specializationConsts[arrayDef.stageIdx][arrayDef.dimension]))
            {
                Logger::error("ShaderParameters", "%s() : Sampler %s array count specialization invalid"
                    , __func__, samplerParamDesc->samplerEntryPtr->attributeName.c_str());
            }

            SamplerParameterData& paramData = shaderSamplers[samplerParamDesc->samplerEntryPtr->attributeName];
            paramData.samplers.resize(count);
            paramData.descriptorInfo = samplerParamDesc;
        }
    }
}

void ShaderParameters::release()
{
    BaseType::release();

    for (const std::pair<const String, BufferParametersData>& bufferParam : shaderBuffers)
    {
        delete[] bufferParam.second.cpuBuffer;
        if (bufferParam.second.gpuBuffer && !bufferParam.second.bIsExternal)
        {
            bufferParam.second.gpuBuffer->release();
            delete bufferParam.second.gpuBuffer;
        }
    }
    shaderBuffers.clear();
    shaderTexels.clear();
    shaderTextures.clear();
    shaderSamplers.clear();
}

String ShaderParameters::getResourceName() const
{
    return descriptorSetName;
}

void ShaderParameters::setResourceName(const String& name)
{
    descriptorSetName = name;
}

std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> ShaderParameters::getAllReadOnlyTextures() const
{
    std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> textures;
    for (const std::pair<const String, TextureParameterData>& textuteParam : shaderTextures)
    {
        for (const auto& img : textuteParam.second.textures)
        {
            if (img.texture->isShaderRead() && !img.texture->isShaderWrite())
            {
                textures.emplace_back(std::pair<ImageResource*, const ShaderTextureDescriptorType*>{ img.texture, textuteParam.second.descriptorInfo });
            }
        }
    }
    return textures;
}

std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> ShaderParameters::getAllReadOnlyBuffers() const
{
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> buffers;
    for (const std::pair<const String, BufferParametersData>& bufferParam : shaderBuffers)
    {
        if (!bufferParam.second.descriptorInfo->bIsStorage && bufferParam.second.gpuBuffer->isTypeOf<GraphicsRBuffer>())
        {
            buffers.emplace_back(std::pair<BufferResource*, const ShaderBufferDescriptorType*>{bufferParam.second.gpuBuffer, bufferParam.second.descriptorInfo });
        }
    }
    return buffers;
}

std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> ShaderParameters::getAllReadOnlyTexels() const
{
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> buffers;
    for (const std::pair<const String, TexelParameterData>& bufferParam : shaderTexels)
    {
        for (BufferResource* texels : bufferParam.second.gpuBuffers)
        {
            if (!bufferParam.second.descriptorInfo->bIsStorage && texels->isTypeOf<GraphicsRTexelBuffer>())
            {
                buffers.emplace_back(std::pair<BufferResource*, const ShaderBufferDescriptorType*>{ texels, bufferParam.second.descriptorInfo });
            }
        }
    }
    return buffers;
}

std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> ShaderParameters::getAllWriteTextures() const
{
    std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> textures;
    for (const std::pair<const String, TextureParameterData>& textuteParam : shaderTextures)
    {
        for (const auto& img : textuteParam.second.textures)
        {
            if (img.texture->isShaderWrite())
            {
                textures.emplace_back(std::pair<ImageResource*, const ShaderTextureDescriptorType*>{ img.texture, textuteParam.second.descriptorInfo });
            }
        }
    }
    return textures;
}

std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> ShaderParameters::getAllWriteBuffers() const
{
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> buffers;
    for (const std::pair<const String, BufferParametersData>& bufferParam : shaderBuffers)
    {
        if (bufferParam.second.descriptorInfo->bIsStorage || bufferParam.second.gpuBuffer->isTypeOf<GraphicsWBuffer>() || bufferParam.second.gpuBuffer->isTypeOf<GraphicsRWBuffer>())
        {
            buffers.emplace_back(std::pair<BufferResource*, const ShaderBufferDescriptorType*>{bufferParam.second.gpuBuffer, bufferParam.second.descriptorInfo });
        }
    }
    return buffers;
}

std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> ShaderParameters::getAllWriteTexels() const
{
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> buffers;
    for (const std::pair<const String, TexelParameterData>& bufferParam : shaderTexels)
    {
        for (BufferResource* texels : bufferParam.second.gpuBuffers)
        {
            if (bufferParam.second.descriptorInfo->bIsStorage || texels->isTypeOf<GraphicsWTexelBuffer>() || texels->isTypeOf<GraphicsRWTexelBuffer>())
            {
                buffers.emplace_back(std::pair<BufferResource*, const ShaderBufferDescriptorType*>{ texels, bufferParam.second.descriptorInfo });
            }
        }
    }
    return buffers;
}

void ShaderParameters::updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    std::vector<BatchCopyBufferData> copies;
    {
        std::unordered_set<BufferParameterUpdate, BufferParameterUpdate::Hasher> uniqueBufferUpdates(bufferUpdates.cbegin(), bufferUpdates.cend());

        for (const BufferParameterUpdate& bufferUpdate : uniqueBufferUpdates)
        {
            const BufferParametersData& bufferParamData = shaderBuffers.at(bufferUpdate.bufferName);
            const BufferParametersData::BufferParameter& bufferParamField = bufferParamData.bufferParams.at(bufferUpdate.paramName);

            // Offset of struct when updating field is inside inner struct, 
            // In which case field offset will always be from its outer struct and we have to add this offset to that for obtaining proper offset
            uint32 outerOffset = 0;
            {
                const BufferParametersData::BufferParameter* outerBufferParamField = bufferParamField.outerName.empty()
                    ? nullptr : &bufferParamData.bufferParams.at(bufferParamField.outerName);
                while (outerBufferParamField)
                {
                    if (outerBufferParamField->bufferField->bIsArray)
                    {
                        Logger::warn("ShaderParameters", "%s(): Setting value of parameter[%s] inside a struct[%s] in AoS[%s] will always set param value at struct index 0", __func__
                            , bufferUpdate.paramName.getChar(), bufferUpdate.bufferName.getChar(), outerBufferParamField->bufferField->paramName.getChar());
                    }

                    outerOffset += outerBufferParamField->bufferField->offset;
                    outerBufferParamField = outerBufferParamField->outerName.empty()
                        ? nullptr : &bufferParamData.bufferParams.at(outerBufferParamField->outerName);
                }
            }

            BatchCopyBufferData copyData;
            copyData.dst = bufferParamData.gpuBuffer;
            copyData.dstOffset = outerOffset + bufferParamField.bufferField->offset + (bufferUpdate.index * bufferParamField.bufferField->stride);
            copyData.dataToCopy = bufferParamField.bufferField->fieldData(bufferParamField.outerPtr, nullptr, &copyData.size);
            copyData.dataToCopy = reinterpret_cast<const uint8*>(copyData.dataToCopy) + (bufferUpdate.index * copyData.size);

            copies.emplace_back(copyData);
        }
    }
    ParamUpdateLambdaOut genericUpdateOut{ &copies };
    for (const ParamUpdateLambda& lambda : genericUpdates)
    {
        lambda(genericUpdateOut, cmdList, graphicsInstance);
    }
    bufferUpdates.clear();
    genericUpdates.clear();

    if (!copies.empty())
    {
        cmdList->copyToBuffer(copies);
    }
}

std::pair<const ShaderParameters::BufferParametersData*, const ShaderParameters::BufferParametersData::BufferParameter*> 
    ShaderParameters::findBufferParam(String& bufferName, const String& paramName) const
{
    std::pair<const BufferParametersData*, const BufferParametersData::BufferParameter*> retVal{ nullptr, nullptr };

    for (const std::pair<const String, BufferParametersData>& bufferParams : shaderBuffers)
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

template<typename FieldType>
bool ShaderParameters::setFieldParam(const String& paramName, const FieldType& value, uint32 index)
{
    bool bValueSet = false;
    String bufferName;
    std::pair<const BufferParametersData*, const BufferParametersData::BufferParameter*> foundInfo =
        findBufferParam(bufferName, paramName);

    BufferParameterUpdate updateVal{ bufferName, paramName, 0 };

    if (foundInfo.first && foundInfo.second && !foundInfo.second->bufferField->bIsStruct)
    {
        if (foundInfo.second->bufferField->bIsArray)
        {
            bValueSet = foundInfo.second->bufferField->setFieldDataArray(foundInfo.second->outerPtr, value, index);
            updateVal.index = index;
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
    return bValueSet;
}

template<typename FieldType>
bool ShaderParameters::setFieldParam(const String& paramName, const String& bufferName, const FieldType& value, uint32 index)
{
    bool bValueSet = false;
    BufferParameterUpdate updateVal{ bufferName, paramName, 0 };

    auto bufferParamsItr = shaderBuffers.find(bufferName);
    if (bufferParamsItr != shaderBuffers.end())
    {
        auto bufferParamItr = bufferParamsItr->second.bufferParams.find(paramName);
        if (bufferParamItr != bufferParamsItr->second.bufferParams.end() && !bufferParamItr->second.bufferField->bIsStruct)
        {
            if (bufferParamItr->second.bufferField->bIsArray)
            {
                bValueSet = bufferParamItr->second.bufferField->setFieldDataArray(bufferParamItr->second.outerPtr, value, index);
                updateVal.index = index;
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
    return bValueSet;
}

template<typename FieldType>
FieldType ShaderParameters::getFieldParam(const String& paramName, uint32 index) const
{
    String bufferName;
    std::pair<const BufferParametersData*, const BufferParametersData::BufferParameter*> foundInfo =
        findBufferParam(bufferName, paramName);

    if (foundInfo.first && foundInfo.second && !foundInfo.second->bufferField->bIsStruct)
    {
        uint32 fieldTypeSize;
        void* dataPtr = foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, &fieldTypeSize);
        if (sizeof(FieldType) == fieldTypeSize)
        {
            uint32 idx = foundInfo.second->bufferField->bIsArray ? index : 0;
            return reinterpret_cast<FieldType*>(dataPtr)[idx];
        }
    }
    return FieldType(0);
}

template<typename FieldType>
FieldType ShaderParameters::getFieldParam(const String& paramName, const String& bufferName, uint32 index) const
{
    auto bufferParamsItr = shaderBuffers.find(bufferName);
    if (bufferParamsItr != shaderBuffers.end())
    {
        auto bufferParamItr = bufferParamsItr->second.bufferParams.find(paramName);
        if (bufferParamItr != bufferParamsItr->second.bufferParams.end() && !bufferParamItr->second.bufferField->bIsStruct)
        {
            uint32 fieldTypeSize;
            void* dataPtr = bufferParamItr->second.bufferField->fieldData(bufferParamItr->second.outerPtr, nullptr, &fieldTypeSize);
            if (sizeof(FieldType) == fieldTypeSize)
            {
                uint32 idx = bufferParamItr->second.bufferField->bIsArray ? index : 0;
                return reinterpret_cast<FieldType*>(dataPtr)[idx];
            }
        }
    }
    return FieldType(0);
}

bool ShaderParameters::setIntParam(const String& paramName, const String& bufferName, int32 value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setIntParam(const String& paramName, const String& bufferName, uint32 value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setIntParam(const String& paramName, int32 value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setIntParam(const String& paramName, uint32 value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setFloatParam(const String& paramName, const String& bufferName, float value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setFloatParam(const String& paramName, float value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setVector2Param(const String& paramName, const String& bufferName, const Vector2D& value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setVector2Param(const String& paramName, const Vector2D& value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setVector4Param(const String& paramName, const String& bufferName, const Vector4D& value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setVector4Param(const String& paramName, const Vector4D& value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setMatrixParam(const String& paramName, const String& bufferName, const Matrix4& value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, bufferName, value, index);
}

bool ShaderParameters::setMatrixParam(const String& paramName, const Matrix4& value, uint32 index/* = 0 */)
{
    return setFieldParam(paramName, value, index);
}

bool ShaderParameters::setBufferResource(const String& paramName, BufferResource* buffer)
{
    auto bufferDataItr = shaderBuffers.find(paramName);
    if (bufferDataItr != shaderBuffers.end())
    {
        if (bufferDataItr->second.gpuBuffer && !bufferDataItr->second.bIsExternal)
        {
            bufferDataItr->second.gpuBuffer->release();
            delete bufferDataItr->second.gpuBuffer;
        }
        bufferDataItr->second.bIsExternal = true;
        bufferDataItr->second.gpuBuffer = buffer;
        bufferResourceUpdates.insert(paramName);
    }
    return false;
}

bool ShaderParameters::setTexelParam(const String& paramName, BufferResource* texelBuffer, uint32 index/* = 0 */)
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

bool ShaderParameters::setTextureParam(const String& paramName, ImageResource* texture, SharedPtr<SamplerInterface> sampler, uint32 index/* = 0 */)
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

bool ShaderParameters::setTextureParam(const String& paramName, ImageResource* texture, uint32 index/* = 0 */)
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

bool ShaderParameters::setTextureParamViewInfo(const String& paramName, const ImageViewInfo& textureViewInfo, uint32 index /*= 0*/)
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

bool ShaderParameters::setSamplerParam(const String& paramName, SharedPtr<SamplerInterface> sampler, uint32 index/* = 0 */)
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

int32 ShaderParameters::getIntParam(const String& paramName, const String& bufferName, uint32 index/* = 0 */) const
{
    return getFieldParam<int32>(paramName, bufferName, index);
}

int32 ShaderParameters::getIntParam(const String& paramName, uint32 index/* = 0 */) const
{
    return getFieldParam<int32>(paramName, index);
}

uint32 ShaderParameters::getUintParam(const String& paramName, const String& bufferName, uint32 index/* = 0 */) const
{
    return getFieldParam<uint32>(paramName, bufferName, index);
}

uint32 ShaderParameters::getUintParam(const String& paramName, uint32 index/* = 0 */) const
{
    return getFieldParam<uint32>(paramName, index);
}

float ShaderParameters::getFloatParam(const String& paramName, const String& bufferName, uint32 index/* = 0 */) const
{
    return getFieldParam<float>(paramName, bufferName, index);
}

float ShaderParameters::getFloatParam(const String& paramName, uint32 index/* = 0 */) const
{
    return getFieldParam<float>(paramName, index);
}

Vector2D ShaderParameters::getVector2Param(const String& paramName, const String& bufferName, uint32 index/* = 0 */) const
{
    return getFieldParam<Vector2D>(paramName, bufferName, index);
}

Vector2D ShaderParameters::getVector2Param(const String& paramName, uint32 index/* = 0 */) const
{
    return getFieldParam<Vector2D>(paramName, index);
}

Vector4D ShaderParameters::getVector4Param(const String& paramName, const String& bufferName, uint32 index/* = 0 */) const
{
    return getFieldParam<Vector4D>(paramName, bufferName, index);
}

Vector4D ShaderParameters::getVector4Param(const String& paramName, uint32 index/* = 0 */) const
{
    return getFieldParam<Vector4D>(paramName, index);
}

Matrix4 ShaderParameters::getMatrixParam(const String& paramName, uint32 index/* = 0 */) const
{
    return getFieldParam<Matrix4>(paramName, index);
}

Matrix4 ShaderParameters::getMatrixParam(const String& paramName, const String& bufferName, uint32 index/* = 0 */) const
{
    return getFieldParam<Matrix4>(paramName, index);
}

BufferResource* ShaderParameters::getBufferResource(const String& paramName)
{
    auto bufferDataItr = shaderBuffers.find(paramName);
    return (bufferDataItr != shaderBuffers.end()) ? bufferDataItr->second.gpuBuffer : nullptr;
}

BufferResource* ShaderParameters::getTexelParam(const String& paramName, uint32 index/* = 0 */) const
{
    auto texelParamItr = shaderTexels.find(paramName);
    if (texelParamItr != shaderTexels.end() && texelParamItr->second.gpuBuffers.size() > index)
    {
        return texelParamItr->second.gpuBuffers[index];
    }
    return nullptr;
}

ImageResource* ShaderParameters::getTextureParam(const String& paramName, uint32 index/* = 0 */) const
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.cend() && textureParamItr->second.textures.size() > index)
    {
        return textureParamItr->second.textures[index].texture;
    }
    return nullptr;
}

ImageResource* ShaderParameters::getTextureParam(SharedPtr<SamplerInterface>& outSampler, const String& paramName, uint32 index/* = 0 */) const
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

SharedPtr<SamplerInterface> ShaderParameters::getSamplerParam(const String& paramName, uint32 index/* = 0 */) const
{
    auto samplerParamItr = shaderSamplers.find(paramName);
    if (samplerParamItr != shaderSamplers.cend() && samplerParamItr->second.samplers.size() > index)
    {
        return samplerParamItr->second.samplers[index];
    }
    return nullptr;
}
