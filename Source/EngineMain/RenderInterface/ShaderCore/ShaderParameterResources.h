#pragma once
#include "../Resources/GraphicsResources.h"
#include "ShaderDataTypes.h"
#include "ShaderParameters.h"
#include "../../Core/Types/Functions.h"
#include "../Rendering/IRenderCommandList.h"

#include <map>
#include <set>

class ShaderResource;
class BufferResource;
class ImageResource;
class IRenderCommandList;
class IGraphicsInstance;
class Vector2D;
class Vector4D;
class Matrix4;

/**
* ShaderDescriptorParamType - Wrapper for param type of an descriptors entry of any set. Currently it can be either a texture or buffer or sampler
*
* @author Jeslas Pravin
*
* @date June 2020
*/
struct ShaderDescriptorParamType
{
protected:
    enum Type
    {
        Texture,
        Buffer,
        Sampler
    };

    Type paramType;

public:
    virtual ~ShaderDescriptorParamType() = default;

    static void wrapReflectedDescriptors(std::map<String, ShaderDescriptorParamType*>& descriptorParams, const ReflectDescriptorBody& reflectDescriptors
        , std::map<String, struct ShaderBufferDescriptorType*>* filterBufferDescriptors = nullptr);

    template<typename DescriptorParamType>
    friend const DescriptorParamType* Cast(const ShaderDescriptorParamType* shaderDescriptorType);
    template<typename DescriptorParamType>
    friend DescriptorParamType* Cast(ShaderDescriptorParamType* shaderDescriptorType);
};

struct ShaderTextureDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Texture;

    ShaderTextureDescriptorType();

    const DescEntryTexture* textureEntryPtr;

    // If sampling or storing
    EImageShaderUsage::Type imageUsageFlags;
    bool bIsAttachedSampler;
};

struct ShaderBufferDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Buffer;

    ShaderBufferDescriptorType();

    // Buffer parameters info that specify the internal structure of buffer this will be filled with offset and informations from reflection
    ShaderBufferParamInfo* bufferParamInfo;
    const DescEntryBuffer* bufferEntryPtr = nullptr;

    const DescEntryTexelBuffer* texelBufferEntryPtr = nullptr;

    // C++ size of buffer as this gets lost when bufferParamInfo gets filled with GPU stride and offsets
    uint32 bufferNativeStride;
    bool bIsStorage;
};

struct ShaderSamplerDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Sampler;

    ShaderSamplerDescriptorType();

    const DescEntrySampler* samplerEntryPtr;
};

// Contains shader's API specific layout informations for a descriptors set, Push constants informations
// Assumption : 
//  Descriptors set 0 will be common for scene so only one layout will be available in global context
//  Descriptors set 1 will be common for a vertex type instance so layout will be available in vertex specific type objects
//  Descriptors set 2 will be unique for each shader
class ShaderSetParametersLayout : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderSetParametersLayout, , GraphicsResource, )

protected:
    // Since 2nd set is only of interest for this object
    uint32 shaderSetID;

    const ShaderResource* respectiveShaderRes;
    std::map<String, ShaderDescriptorParamType*> paramsLayout;

protected:
    ShaderSetParametersLayout() = default;
    ShaderSetParametersLayout(const ShaderResource* shaderResource, uint32 setID);

    // Bind buffer info so it can be filled with offset, stride and size informations
    virtual void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const {};
public:    

    /* GraphicsResource overrides */
    virtual void init() override;
    virtual void release() override;
    /* Override ends */

    const ShaderDescriptorParamType* parameterDescription(uint32& outSetIdx, const String& paramName) const;
    const ShaderDescriptorParamType* parameterDescription(const String& paramName) const;
    const std::map<String, ShaderDescriptorParamType*>& allParameterDescriptions() const;
    uint32 getSetID() const { return shaderSetID; }
    const ShaderResource* getShaderResource() const { return respectiveShaderRes; }
};

// Contains shader's all descriptors set layouts
class ShaderParametersLayout : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderParametersLayout, , GraphicsResource, )

protected:

    const ShaderResource* respectiveShaderRes;
    /*
    * Assumed that all descriptors in the shader has unique name irrespective of set just like vertex attributes
    * uint32 in pair is descriptor set Index
    */
    std::map<String, std::pair<uint32, ShaderDescriptorParamType*>> paramsLayout;

protected:
    ShaderParametersLayout() = default;
    ShaderParametersLayout(const ShaderResource* shaderResource);
public:

    /* GraphicsResource overrides */
    virtual void init() override;
    virtual void release() override;
    /* Override ends */

    const ShaderDescriptorParamType* parameterDescription(uint32& outSetIdx, const String& paramName) const;
    const ShaderDescriptorParamType* parameterDescription(const String& paramName) const;
    std::map<String, ShaderDescriptorParamType*> allParameterDescriptions() const;
    uint32 getSetID(const String& paramName) const;
    const ShaderResource* getShaderResource() const { return respectiveShaderRes; }
};

class ShaderParameters : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderParameters, , GraphicsResource, )

protected:

    struct ParamUpdateLambdaOut
    {
        std::vector<struct BatchCopyBufferData>* bufferUpdates;
    };

    using ParamUpdateLambda = LambdaFunction<void, ParamUpdateLambdaOut&, IRenderCommandList*, IGraphicsInstance*>;
    struct BufferParameterUpdate
    {
        String bufferName;
        String paramName;
        uint32 index = 0;

        bool operator==(const BufferParameterUpdate& other) const
        {
            return bufferName == other.bufferName && paramName == other.paramName && index == other.index;
        }
        struct Hasher 
        {
            _NODISCARD size_t operator()(const BufferParameterUpdate& keyVal) const noexcept;
        };
    };

    struct BufferParametersData
    {
        struct BufferParameter
        {
            void* outerPtr = nullptr;
            const ShaderBufferField* bufferField = nullptr;
        };

        // If the buffer resource is set manually, then don't manage it here 
        bool bIsExternal = false;
        const ShaderBufferDescriptorType* descriptorInfo;
        uint8* cpuBuffer;
        BufferResource* gpuBuffer = nullptr;

        std::map<String, BufferParameter> bufferParams;
    };

    struct TexelParameterData
    {
        const ShaderBufferDescriptorType* descriptorInfo;
        std::vector<BufferResource*> gpuBuffers;
    };

    struct TextureParameterData
    {
        struct TextureViewAndSampler
        {
            ImageResource* texture = nullptr;
            ImageViewInfo viewInfo;
            // Optional sampler
            SharedPtr<class SamplerInterface> sampler;
        };
        const ShaderTextureDescriptorType* descriptorInfo;
        
        std::vector<TextureViewAndSampler> textures;
    };

    struct SamplerParameterData
    {
        const ShaderSamplerDescriptorType* descriptorInfo;

        std::vector<SharedPtr<class SamplerInterface>> samplers;
    };

    std::map<String, BufferParametersData> shaderBuffers;
    std::map<String, TexelParameterData> shaderTexels;
    std::map<String, TextureParameterData> shaderTextures;
    std::map<String, SamplerParameterData> shaderSamplers;
    // will be used only in terms of ShaderParametersLayout
    std::set<uint32> ignoredSets;

    std::vector<BufferParameterUpdate> bufferUpdates;
    std::set<String> bufferResourceUpdates;
    std::set<std::pair<String, uint32>> texelUpdates;
    std::set<std::pair<String, uint32>> textureUpdates;
    std::set<std::pair<String, uint32>> samplerUpdates;
    std::vector<ParamUpdateLambda> genericUpdates;

    const GraphicsResource* paramLayout;
    String descriptorSetName;
private:
    void initBufferParams(BufferParametersData& bufferParamData, const ShaderBufferParamInfo* bufferParamInfo, void* outerPtr, bool bIsNested) const;
    void initParamsMaps(const std::map<String, ShaderDescriptorParamType*>& paramsDesc, const std::vector<std::vector<SpecializationConstantEntry>>& specializationConsts);
    std::pair<const BufferParametersData*, const BufferParametersData::BufferParameter*> findBufferParam(String& bufferName, const String& paramName) const;
    template<typename FieldType>
    bool setFieldParam(const String& paramName, const FieldType& value, uint32 index);
    template<typename FieldType>
    bool setFieldParam(const String& paramName, const String& bufferName, const FieldType& value, uint32 index);
    template<typename FieldType>
    FieldType getFieldParam(const String& paramName, uint32 index) const;
    template<typename FieldType>
    FieldType getFieldParam(const String& paramName, const String& bufferName, uint32 index) const;
protected:
    ShaderParameters() = default;
    ShaderParameters(const GraphicsResource* shaderParamLayout, const std::set<uint32>& ignoredSetIds = {});
public:
    /* GraphicsResource overrides */
    virtual void init() override;
    virtual void release() override;
    String getResourceName() const final;
    void setResourceName(const String& name) final;
    /* Override ends */

    const GraphicsResource* getParamLayout() const { return paramLayout; }

    // Read only
    std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> getAllReadOnlyTextures() const;
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> getAllReadOnlyBuffers() const;
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> getAllReadOnlyTexels() const;
    // Read Write and Write
    std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> getAllWriteTextures() const;
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> getAllWriteBuffers() const;
    std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> getAllWriteTexels() const;

    virtual void updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    bool setIntParam(const String& paramName, int32 value, uint32 index = 0);
    bool setIntParam(const String& paramName, uint32 value, uint32 index = 0);
    bool setFloatParam(const String& paramName, float value, uint32 index = 0);
    bool setVector2Param(const String& paramName, const Vector2D& value, uint32 index = 0);
    bool setVector4Param(const String& paramName, const Vector4D& value, uint32 index = 0);
    bool setMatrixParam(const String& paramName, const Matrix4& value, uint32 index = 0);
    bool setIntParam(const String& paramName, const String& bufferName, int32 value, uint32 index = 0);
    bool setIntParam(const String& paramName, const String& bufferName, uint32 value, uint32 index = 0);
    bool setFloatParam(const String& paramName, const String& bufferName, float value, uint32 index = 0);
    bool setVector2Param(const String& paramName, const String& bufferName, const Vector2D& value, uint32 index = 0);
    bool setVector4Param(const String& paramName, const String& bufferName, const Vector4D& value, uint32 index = 0);
    bool setMatrixParam(const String& paramName, const String& bufferName, const Matrix4& value, uint32 index = 0);
    template<typename BufferType>
    bool setBuffer(const String& paramName, const BufferType& bufferValue, uint32 index = 0);
    bool setBufferResource(const String& paramName, BufferResource* buffer);
    bool setTexelParam(const String& paramName, BufferResource* texelBuffer, uint32 index = 0);
    bool setTextureParam(const String& paramName, ImageResource* texture, uint32 index = 0);
    bool setTextureParam(const String& paramName, ImageResource* texture, SharedPtr<SamplerInterface> sampler, uint32 index = 0);
    bool setTextureParamViewInfo(const String& paramName, const ImageViewInfo& textureViewInfo, uint32 index = 0);
    bool setSamplerParam(const String& paramName, SharedPtr<SamplerInterface> sampler, uint32 index = 0);

    int32 getIntParam(const String& paramName, uint32 index = 0) const;
    uint32 getUintParam(const String& paramName, uint32 index = 0) const;
    float getFloatParam(const String& paramName, uint32 index = 0) const;
    Vector2D getVector2Param(const String& paramName, uint32 index = 0) const;
    Vector4D getVector4Param(const String& paramName, uint32 index = 0) const;
    Matrix4 getMatrixParam(const String& paramName, uint32 index = 0) const;
    int32 getIntParam(const String& paramName, const String& bufferName, uint32 index = 0) const;
    uint32 getUintParam(const String& paramName, const String& bufferName, uint32 index = 0) const;
    float getFloatParam(const String& paramName, const String& bufferName, uint32 index = 0) const;
    Vector2D getVector2Param(const String& paramName, const String& bufferName, uint32 index = 0) const;
    Vector4D getVector4Param(const String& paramName, const String& bufferName, uint32 index = 0) const;
    Matrix4 getMatrixParam(const String& paramName, const String& bufferName, uint32 index = 0) const;
    BufferResource* getBufferResource(const String& paramName);
    BufferResource* getTexelParam(const String& paramName, uint32 index = 0) const;
    ImageResource* getTextureParam(const String& paramName, uint32 index = 0) const;
    ImageResource* getTextureParam(SharedPtr<SamplerInterface>& outSampler, const String& paramName, uint32 index = 0) const;
    SharedPtr<SamplerInterface> getSamplerParam(const String& paramName, uint32 index = 0) const;
};

template<typename BufferType>
bool ShaderParameters::setBuffer(const String& paramName, const BufferType& bufferValue, uint32 index/* = 0 */)
{
    bool bValueSet = false;
    auto bufferDataItr = shaderBuffers.find(paramName);

    if (bufferDataItr == shaderBuffers.end())
    {
        String bufferName;
        std::pair<const BufferParametersData*, const BufferParametersData::BufferParameter*> foundInfo = findBufferParam(bufferName, paramName);
        if (foundInfo.first && foundInfo.second && foundInfo.second->bufferField->bIsStruct)
        {
            if (foundInfo.second->bufferField->bIsArray)
            {
                if (bValueSet = foundInfo.second->bufferField->setFieldDataArray(foundInfo.second->outerPtr, bufferValue, index))
                {
                    genericUpdates.emplace_back([foundInfo, index](ParamUpdateLambdaOut& paramOut, IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
                        {
                            BufferType* bufferPtr = reinterpret_cast<BufferType*>(foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, nullptr));
                            cmdList->recordCopyToBuffer<BufferType>(*paramOut.bufferUpdates, foundInfo.first->gpuBuffer
                                , foundInfo.second->bufferField->offset + (index * foundInfo.second->bufferField->stride)
                                , bufferPtr + index, foundInfo.second->bufferField->paramInfo);
                        });
                }
            }
            else if (bValueSet = foundInfo.second->bufferField->setFieldData(foundInfo.second->outerPtr, bufferValue))
            {
                genericUpdates.emplace_back([foundInfo](ParamUpdateLambdaOut& paramOut, IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
                    {
                        cmdList->recordCopyToBuffer<BufferType>(*paramOut.bufferUpdates, foundInfo.first->gpuBuffer
                            , foundInfo.second->bufferField->offset
                            , reinterpret_cast<BufferType*>(foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, nullptr))
                            , foundInfo.second->bufferField->paramInfo);
                    });
            }
        }
    }
    else
    {
        BufferParametersData* bufferDataPtr = &bufferDataItr->second;
        if (bValueSet = (bufferDataPtr->descriptorInfo->bufferNativeStride == sizeof(BufferType)))
        {
            (*reinterpret_cast<BufferType*>(bufferDataPtr->cpuBuffer)) = bufferValue;
            genericUpdates.emplace_back([bufferDataPtr](ParamUpdateLambdaOut& paramOut, IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
                {
                    cmdList->recordCopyToBuffer<BufferType>(*paramOut.bufferUpdates, bufferDataPtr->gpuBuffer
                        , 0, reinterpret_cast<BufferType*>(bufferDataPtr->cpuBuffer), bufferDataPtr->descriptorInfo->bufferParamInfo);
                });
        }
    }
    return bValueSet;
}
