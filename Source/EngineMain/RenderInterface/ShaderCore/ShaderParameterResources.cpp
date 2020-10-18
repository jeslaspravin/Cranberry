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
        Logger::warn("DescriptorTypeParams", "%s : Sub pass inputs are not supported yet %s", __func__, descriptorInfo.attributeName.c_str());
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
    // Fill those bound buffer info with GPU reflect data
    for (const std::pair<const String, ShaderBufferDescriptorType*>& bufferDescWrapper : bufferDescriptors)
    {
        bufferDescWrapper.second->bufferNativeStride = bufferDescWrapper.second->bufferParamInfo->paramStride();
        ShaderParameterUtility::fillRefToBufParamInfo(*bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data);
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
    Logger::error("ShaderSetParametersLayout", "%s : Parameter %s is not available in shader %s at set %u", __func__
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
    for (const std::pair<const String, ShaderBufferDescriptorType*>& bufferDescWrapper : bufferDescriptors)
    {
        bufferDescWrapper.second->bufferNativeStride = bufferDescWrapper.second->bufferParamInfo->paramStride();
        ShaderParameterUtility::fillRefToBufParamInfo(*bufferDescWrapper.second->bufferParamInfo, bufferDescWrapper.second->bufferEntryPtr->data.data);
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
    Logger::error("ShaderParametersLayout", "%s : Parameter %s is not available in shader %s", __func__
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
    return seed;
}

ShaderParameters::ShaderParameters(const GraphicsResource* shaderParamLayout, const std::set<uint32>& ignoredSetIds/* = {}*/)
    : GraphicsResource()
    , paramLayout(shaderParamLayout)
    , ignoredSets(ignoredSetIds)
{
    if (paramLayout->getType()->isChildOf<ShaderSetParametersLayout>())
    {
        initParamsMaps(static_cast<const ShaderSetParametersLayout*>(paramLayout)->allParameterDescriptions());
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
        initParamsMaps(allParameters);
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
        bufferParameters.second.gpuBuffer->setResourceName(bufferParameters.first);
        bufferParameters.second.gpuBuffer->init();
    }
}

void ShaderParameters::initBufferParams(BufferParametersData& bufferParamData, const ShaderBufferParamInfo* bufferParamInfo, void* outerPtr) const
{
    const ShaderBufferFieldNode* currentNode = &bufferParamInfo->startNode;
    while (currentNode->isValid())
    {
        debugAssert(!currentNode->field->bIsArray);
        bufferParamData.bufferParams[currentNode->field->paramName] = { outerPtr, currentNode->field };
        if (currentNode->field->bIsStruct)
        {
            void* nextOuterPtr = nullptr;
            {
                uint32 dummy;
                nextOuterPtr = currentNode->field->fieldData(dummy, outerPtr);
            }
            initBufferParams(bufferParamData, currentNode->field->paramInfo, nextOuterPtr);
        }
        currentNode = currentNode->nextNode;
    }
}

void ShaderParameters::initParamsMaps(const std::map<String, ShaderDescriptorParamType*>& paramsDesc)
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
                if(bufferParamDesc->bIsStorage)
                {
                    paramData.gpuBuffer = new GraphicsWBuffer(bufferParamDesc->bufferParamInfo->paramStride());
                }
                else 
                {
                    paramData.gpuBuffer = new GraphicsRBuffer(bufferParamDesc->bufferParamInfo->paramStride());
                }

                initBufferParams(paramData, bufferParamDesc->bufferParamInfo, paramData.cpuBuffer);
                shaderBuffers[bufferParamDesc->bufferEntryPtr->attributeName] = paramData;
            }
            else
            {
                debugAssert(bufferParamDesc->texelBufferEntryPtr->data.data.arraySize.size() == 1 && bufferParamDesc->texelBufferEntryPtr->data.data.arraySize[0].dimension == 1);
                TexelParameterData paramData;
                paramData.descriptorInfo = bufferParamDesc;
                shaderTexels[bufferParamDesc->texelBufferEntryPtr->attributeName] = paramData;
            }
        }
        else if (const ShaderTextureDescriptorType* textureParamDesc = Cast<ShaderTextureDescriptorType>(paramDesc.second))
        {
            TextureParameterData paramData;
            paramData.descriptorInfo = textureParamDesc;

            debugAssert(textureParamDesc->textureEntryPtr->data.data.arraySize.size() == 1 && textureParamDesc->textureEntryPtr->data.data.arraySize[0].dimension == 1);
            shaderTextures[textureParamDesc->textureEntryPtr->attributeName] = paramData;
        }
        else if (const ShaderSamplerDescriptorType* samplerParamDesc = Cast<ShaderSamplerDescriptorType>(paramDesc.second))
        {
            SamplerParameterData paramData;
            paramData.descriptorInfo = samplerParamDesc;

            debugAssert(samplerParamDesc->samplerEntryPtr->data.data.size() == 1 && samplerParamDesc->samplerEntryPtr->data.data[0].dimension == 1);
            shaderSamplers[samplerParamDesc->samplerEntryPtr->attributeName] = paramData;
        }
    }
}

void ShaderParameters::release()
{
    BaseType::release();

    for (const std::pair<const String, BufferParametersData>& bufferParam : shaderBuffers)
    {
        delete[] bufferParam.second.cpuBuffer;
        if (bufferParam.second.gpuBuffer != nullptr)
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

void ShaderParameters::updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    std::vector<BatchCopyBufferData> copies;
    {
        std::unordered_set<BufferParameterUpdate, BufferParameterUpdate::Hasher> uniqueBufferUpdates(bufferUpdates.cbegin(), bufferUpdates.cend());

        for (const BufferParameterUpdate& bufferUpdate : uniqueBufferUpdates)
        {
            const BufferParametersData& bufferParamData = shaderBuffers.at(bufferUpdate.bufferName);
            const BufferParametersData::BufferParameter& bufferParamField = bufferParamData.bufferParams.at(bufferUpdate.paramName);

            BatchCopyBufferData copyData;
            copyData.dst = bufferParamData.gpuBuffer;
            copyData.dstOffset = bufferParamField.bufferField->offset;
            copyData.dataToCopy = bufferParamField.bufferField->fieldData(copyData.size, bufferParamField.outerPtr);

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
bool ShaderParameters::setFieldParam(const String& paramName, const FieldType& value)
{
    String bufferName;
    std::pair<const BufferParametersData*, const BufferParametersData::BufferParameter*> foundInfo =
        findBufferParam(bufferName, paramName);

    if (foundInfo.first && foundInfo.second && !foundInfo.second->bufferField->bIsStruct
        && foundInfo.second->bufferField->setFieldData(foundInfo.second->outerPtr, value))
    {
        bufferUpdates.emplace_back(BufferParameterUpdate{ bufferName, paramName });
        return true;
    }
    return false;
}

template<typename FieldType>
bool ShaderParameters::setFieldParam(const String& paramName, const String& bufferName, const FieldType& value)
{
    auto bufferParamsItr = shaderBuffers.find(bufferName);
    if (bufferParamsItr != shaderBuffers.end())
    {
        auto bufferParamItr = bufferParamsItr->second.bufferParams.find(paramName);
        if (bufferParamItr != bufferParamsItr->second.bufferParams.end() && !bufferParamItr->second.bufferField->bIsStruct
            && bufferParamItr->second.bufferField->setFieldData(bufferParamItr->second.outerPtr, value))
        {
            bufferUpdates.emplace_back(BufferParameterUpdate{ bufferName, paramName });
            return true;
        }
    }
    return false;
}

template<typename FieldType>
FieldType ShaderParameters::getFieldParam(const String& paramName) const
{
    String bufferName;
    std::pair<const BufferParametersData*, const BufferParametersData::BufferParameter*> foundInfo =
        findBufferParam(bufferName, paramName);

    if (foundInfo.first && foundInfo.second && !foundInfo.second->bufferField->bIsStruct)
    {
        uint32 typeSize;
        void* dataPtr = foundInfo.second->bufferField->fieldData(typeSize, foundInfo.second->outerPtr);
        if (sizeof(FieldType) == typeSize)
        {
            return *reinterpret_cast<FieldType*>(dataPtr);
        }
    }
    return FieldType(0);
}

template<typename FieldType>
FieldType ShaderParameters::getFieldParam(const String& paramName, const String& bufferName) const
{
    auto bufferParamsItr = shaderBuffers.find(bufferName);
    if (bufferParamsItr != shaderBuffers.end())
    {
        auto bufferParamItr = bufferParamsItr->second.bufferParams.find(paramName);
        if (bufferParamItr != bufferParamsItr->second.bufferParams.end() && !bufferParamItr->second.bufferField->bIsStruct)
        {
            uint32 typeSize;
            void* dataPtr = bufferParamItr->second.bufferField->fieldData(typeSize, bufferParamItr->second.outerPtr);
            if (sizeof(FieldType) == typeSize)
            {
                return *reinterpret_cast<FieldType*>(dataPtr);
            }
        }
    }
    return FieldType(0);
}

bool ShaderParameters::setIntParam(const String& paramName, const String& bufferName, int32 value)
{
    return setFieldParam(paramName, bufferName, value);
}

bool ShaderParameters::setIntParam(const String& paramName, const String& bufferName, uint32 value)
{
    return setFieldParam(paramName, bufferName, value);
}

bool ShaderParameters::setIntParam(const String& paramName, int32 value)
{
    return setFieldParam(paramName, value);
}

bool ShaderParameters::setIntParam(const String& paramName, uint32 value)
{
    return setFieldParam(paramName, value);
}

bool ShaderParameters::setFloatParam(const String& paramName, const String& bufferName, float value)
{
    return setFieldParam(paramName, bufferName, value);
}

bool ShaderParameters::setFloatParam(const String& paramName, float value)
{
    return setFieldParam(paramName, value);
}

bool ShaderParameters::setVector2Param(const String& paramName, const String& bufferName, const Vector2D& value)
{
    return setFieldParam(paramName, bufferName, value);
}

bool ShaderParameters::setVector2Param(const String& paramName, const Vector2D& value)
{
    return setFieldParam(paramName, value);
}

bool ShaderParameters::setVector4Param(const String& paramName, const String& bufferName, const Vector4D& value)
{
    return setFieldParam(paramName, bufferName, value);
}

bool ShaderParameters::setVector4Param(const String& paramName, const Vector4D& value)
{
    return setFieldParam(paramName, value);
}

bool ShaderParameters::setMatrixParam(const String& paramName, const String& bufferName, const Matrix4& value)
{
    return setFieldParam(paramName, bufferName, value);
}

bool ShaderParameters::setMatrixParam(const String& paramName, const Matrix4& value)
{
    return setFieldParam(paramName, value);
}

bool ShaderParameters::setTexelParam(const String& paramName, BufferResource* texelBuffer)
{
    auto texelParamItr = shaderTexels.find(paramName);
    if (texelParamItr != shaderTexels.end())
    {
        if (texelParamItr->second.gpuBuffer != texelBuffer)
        {
            texelParamItr->second.gpuBuffer = texelBuffer;
            texelUpdates.insert(texelParamItr->first);
        }
        return true;
    }
    return false;
}

bool ShaderParameters::setTextureParam(const String& paramName, ImageResource* texture, SharedPtr<SamplerInterface> sampler)
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.end())
    {
        textureParamItr->second.texture = texture;
        textureParamItr->second.sampler = sampler;
        textureUpdates.insert(textureParamItr->first);
        return true;
    }
    return false;
}

bool ShaderParameters::setTextureParam(const String& paramName, ImageResource* texture)
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.end())
    {
        textureParamItr->second.texture = texture;
        textureUpdates.insert(textureParamItr->first);
        return true;
    }
    return false;
}

bool ShaderParameters::setSamplerParam(const String& paramName, SharedPtr<SamplerInterface> sampler)
{
    auto samplerParamItr = shaderSamplers.find(paramName);
    if (samplerParamItr != shaderSamplers.end())
    {
        samplerParamItr->second.sampler = sampler;
        samplerUpdates.insert(samplerParamItr->first);
        return true;
    }
    return false;
}

int32 ShaderParameters::getIntParam(const String& paramName, const String& bufferName) const
{
    return getFieldParam<int32>(paramName, bufferName);
}

int32 ShaderParameters::getIntParam(const String& paramName) const
{
    return getFieldParam<int32>(paramName);
}

uint32 ShaderParameters::getUintParam(const String& paramName, const String& bufferName) const
{
    return getFieldParam<uint32>(paramName, bufferName);
}

uint32 ShaderParameters::getUintParam(const String& paramName) const
{
    return getFieldParam<uint32>(paramName);
}

float ShaderParameters::getFloatParam(const String& paramName, const String& bufferName) const
{
    return getFieldParam<float>(paramName, bufferName);
}

float ShaderParameters::getFloatParam(const String& paramName) const
{
    return getFieldParam<float>(paramName);
}

Vector2D ShaderParameters::getVector2Param(const String& paramName, const String& bufferName) const
{
    return getFieldParam<Vector2D>(paramName, bufferName);
}

Vector2D ShaderParameters::getVector2Param(const String& paramName) const
{
    return getFieldParam<Vector2D>(paramName);
}

Vector4D ShaderParameters::getVector4Param(const String& paramName, const String& bufferName) const
{
    return getFieldParam<Vector4D>(paramName, bufferName);
}

Vector4D ShaderParameters::getVector4Param(const String& paramName) const
{
    return getFieldParam<Vector4D>(paramName);
}

Matrix4 ShaderParameters::getMatrixParam(const String& paramName) const
{
    return getFieldParam<Matrix4>(paramName);
}

Matrix4 ShaderParameters::getMatrixParam(const String& paramName, const String& bufferName) const
{
    return getFieldParam<Matrix4>(paramName);
}

BufferResource* ShaderParameters::getTexelParam(const String& paramName) const
{
    auto texelParamItr = shaderTexels.find(paramName);
    if (texelParamItr != shaderTexels.end())
    {
        return texelParamItr->second.gpuBuffer;
    }
    return nullptr;
}

ImageResource* ShaderParameters::getTextureParam(const String& paramName) const
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.cend())
    {
        return textureParamItr->second.texture;
    }
    return nullptr;
}

ImageResource* ShaderParameters::getTextureParam(SharedPtr<SamplerInterface>& outSampler, const String& paramName) const
{
    auto textureParamItr = shaderTextures.find(paramName);
    if (textureParamItr != shaderTextures.cend())
    {
        outSampler = textureParamItr->second.sampler;
        return textureParamItr->second.texture;
    }
    outSampler = nullptr;
    return nullptr;
}

SharedPtr<SamplerInterface> ShaderParameters::getSamplerParam(const String& paramName) const
{
    auto samplerParamItr = shaderSamplers.find(paramName);
    if (samplerParamItr != shaderSamplers.cend())
    {
        return samplerParamItr->second.sampler;
    }
    return nullptr;
}
