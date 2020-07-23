#include "ShaderParameterResources.h"
#include "ShaderReflected.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../ShaderCore/ShaderParameterUtility.h"
#include "../Shaders/Base/UtilityShaders.h"

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
        Logger::warn("DescriptorTypeParams", "%s : Sub pass inputs are not supported yet %s", __func__, descriptorInfo.attributeName.c_str());
    }
}

template<typename DescriptorParamType>
DescriptorParamType* Cast(const ShaderDescriptorParamType* shaderDescriptorType)
{
    if (DescriptorParamType::PARAM_TYPE == shaderDescriptorType->paramType)
    {
        return static_cast<DescriptorParamType*>(shaderDescriptorType);
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
    // Fill those bound buffer info with GPU reflect data
    for (const std::pair<String, ShaderBufferDescriptorType*>& bufferDescWrapper : bufferDescriptors)
    {
        bufferDescWrapper.second->bufferNativeStride = bufferDescWrapper.second->bufferParamInfo->paramStride();
        ShaderParameterUtility::fillRefToBufParamInfo(*bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data);
    }
}

void ShaderSetParametersLayout::release()
{
    for (const std::pair<String, ShaderDescriptorParamType*>& shaderDescriptorTypeWrapper : paramsLayout)
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
    Logger::error("ShaderSetParametersLayout", "%s : Parameter %s is not available in shader %s at set %u", __func__
        , paramName.getChar(), respectiveShaderRes->getResourceName().getChar(), shaderSetID);
    return nullptr;
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
    for (const std::pair<String, ShaderBufferDescriptorType*>& bufferDescWrapper : bufferDescriptors)
    {
        bufferDescWrapper.second->bufferNativeStride = bufferDescWrapper.second->bufferParamInfo->paramStride();
        ShaderParameterUtility::fillRefToBufParamInfo(*bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data);
    }

    for (const std::pair<uint32, std::map<String, ShaderDescriptorParamType*>>& setToDescriptorsPair : setToParamsLayout)
    {
        for (const std::pair<String, ShaderDescriptorParamType*>& descriptorWrapper : setToDescriptorsPair.second)
        {
            // Since currently we support only one unique name per shader
            fatalAssert(paramsLayout.find(descriptorWrapper.first) == paramsLayout.end(), "Shader descriptor param name must be unique for a shader pipeline");
            paramsLayout.insert({ descriptorWrapper.first, { setToDescriptorsPair.first, descriptorWrapper.second } });
        }
    }
}

void ShaderParametersLayout::release()
{
    for (const std::pair<String, std::pair<uint32, ShaderDescriptorParamType*>>& shaderDescriptorTypeWrapper : paramsLayout)
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
    Logger::error("ShaderParametersLayout", "%s : Parameter %s is not available in shader %s", __func__
        , paramName.getChar(), respectiveShaderRes->getResourceName().getChar());
    return nullptr;
}

const ShaderDescriptorParamType* ShaderParametersLayout::parameterDescription(const String& paramName) const
{
    uint32 temp;
    return parameterDescription(temp, paramName);
}
