/*!
 * \file ShaderParameterResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <map>
#include <set>
#include <optional>
#include <atomic>

#include "Types/Containers/ReferenceCountPtr.h"
#include "RenderInterface/Resources/GraphicsResources.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "ShaderDataTypes.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "Logger/Logger.h"
#include "Reflections/Functions.h"


class ShaderResource;
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

    // This usage is just to show that this is image and not texture, For actual usage in shader whether read or write use readWriteState inside textureEntryPtr
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

    // This storage is just to show that this is buffer and not uniform, For actual usage in shader whether read or write use readWriteState inside bufferEntryPtr
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
class ENGINERENDERER_EXPORT ShaderSetParametersLayout : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderSetParametersLayout, , GraphicsResource, )

protected:
    // Since 2nd set is only of interest for this object
    uint32 shaderSetID;

    const ShaderResource* respectiveShaderRes;
    std::map<String, ShaderDescriptorParamType*> paramsLayout;
    bool bHasBindless;
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
    bool hasBindless() const { return bHasBindless; }
    const ShaderResource* getShaderResource() const { return respectiveShaderRes; }
};

// Contains shader's all descriptors set layouts
class ENGINERENDERER_EXPORT ShaderParametersLayout : public GraphicsResource
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

    virtual bool hasBindless(uint32 setIdx) const { return false; }

    const ShaderDescriptorParamType* parameterDescription(uint32& outSetIdx, const String& paramName) const;
    const ShaderDescriptorParamType* parameterDescription(const String& paramName) const;
    std::map<String, ShaderDescriptorParamType*> allParameterDescriptions() const;
    uint32 getSetID(const String& paramName) const;
    const ShaderResource* getShaderResource() const { return respectiveShaderRes; }
};

class ENGINERENDERER_EXPORT ShaderParameters : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderParameters, , GraphicsResource, )

private:
    std::atomic<uint32> refCounter;
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
            String outerName;
            const ShaderBufferField* bufferField = nullptr;
        };
        struct RuntimeArrayParameter
        {
            String paramName;
            uint32 offset;
            // Current number of supported elements
            uint32 currentSize;
            std::vector<uint8> runtimeArrayCpuBuffer;
        };

        // If the buffer resource is set manually, then don't manage it here 
        bool bIsExternal = false;
        const ShaderBufferDescriptorType* descriptorInfo;
        uint8* cpuBuffer;
        BufferResourceRef gpuBuffer;

        std::map<String, BufferParameter> bufferParams;
        std::optional<RuntimeArrayParameter> runtimeArray;
    };

    struct TexelParameterData
    {
        const ShaderBufferDescriptorType* descriptorInfo;
        std::vector<BufferResourceRef> gpuBuffers;
    };

    struct TextureParameterData
    {
        struct TextureViewAndSampler
        {
            ImageResourceRef texture;
            ImageViewInfo viewInfo;
            // Optional sampler
            SamplerRef sampler;
        };
        const ShaderTextureDescriptorType* descriptorInfo;
        
        std::vector<TextureViewAndSampler> textures;
    };

    struct SamplerParameterData
    {
        const ShaderSamplerDescriptorType* descriptorInfo;

        std::vector<SamplerRef> samplers;
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
    void initBufferParams(BufferParametersData& bufferParamData, const ShaderBufferParamInfo* bufferParamInfo, void* outerPtr, const char* outerName) const;
    // Runtime array related, returns true if runtime array is setup
    bool initRuntimeArrayData(BufferParametersData& bufferParamData) const;

    // Init entire param maps uniforms, textures, samplers, images, buffers
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
    /* ReferenceCountPtr implementation */
    void addRef();
    void removeRef();
    uint32 refCount() const;
    /* GraphicsResource overrides */
    virtual void init() override;
    virtual void release() override;
    String getResourceName() const final;
    void setResourceName(const String& name) final;
    /* Override ends */

    const GraphicsResource* getParamLayout() const { return paramLayout; }

    // Read only
    std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType*>> getAllReadOnlyTextures() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType*>> getAllReadOnlyBuffers() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType*>> getAllReadOnlyTexels() const;
    // Read Write and Write
    std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType*>> getAllWriteTextures() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType*>> getAllWriteBuffers() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType*>> getAllWriteTexels() const;

    virtual void updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    void pullBufferParamUpdates(std::vector<BatchCopyBufferData>& copies, IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    // Resizing must be done at external wrapper buffer name
    void resizeRuntimeBuffer(const String& bufferName, uint32 minSize);
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
    bool setBufferResource(const String& bufferName, BufferResourceRef buffer);
    bool setTexelParam(const String& paramName, BufferResourceRef texelBuffer, uint32 index = 0);
    bool setTextureParam(const String& paramName, ImageResourceRef texture, uint32 index = 0);
    bool setTextureParam(const String& paramName, ImageResourceRef texture, SamplerRef sampler, uint32 index = 0);
    bool setTextureParamViewInfo(const String& paramName, const ImageViewInfo& textureViewInfo, uint32 index = 0);
    bool setSamplerParam(const String& paramName, SamplerRef sampler, uint32 index = 0);

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
    BufferResourceRef getBufferResource(const String& paramName);
    BufferResourceRef getTexelParam(const String& paramName, uint32 index = 0) const;
    ImageResourceRef getTextureParam(const String& paramName, uint32 index = 0) const;
    ImageResourceRef getTextureParam(SamplerRef& outSampler, const String& paramName, uint32 index = 0) const;
    SamplerRef getSamplerParam(const String& paramName, uint32 index = 0) const;
};
using ShaderParametersRef = ReferenceCountPtr<ShaderParameters>;

