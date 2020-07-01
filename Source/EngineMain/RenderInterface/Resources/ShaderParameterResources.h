#pragma once
#include "GraphicsResources.h"
#include "ShaderDataTypes.h"
#include "../ShaderCore/ShaderParameters.h"

#include <map>

class ShaderResource;

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
    friend DescriptorParamType* Cast(const ShaderDescriptorParamType* shaderDescriptorType);
    template<typename DescriptorParamType>
    friend DescriptorParamType* Cast(ShaderDescriptorParamType* shaderDescriptorType);
};

struct ShaderTextureDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Texture;

    ShaderTextureDescriptorType();

    // If sampling or storing
    EImageShaderUsage::Type imageUsageFlags;
    bool bIsAttachedSampler;

    const DescEntryTexture* textureEntryPtr;
};

struct ShaderBufferDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Buffer;

    ShaderBufferDescriptorType();

    bool bIsStorage;
    const DescEntryBuffer* bufferEntryPtr = nullptr;
    // Buffer parameters info that specify the internal structure of buffer this will be filled with offset and informations from reflection
    ShaderBufferParamInfo* bufferParamInfo;

    const DescEntryTexelBuffer* texelBufferEntryPtr = nullptr;
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
};