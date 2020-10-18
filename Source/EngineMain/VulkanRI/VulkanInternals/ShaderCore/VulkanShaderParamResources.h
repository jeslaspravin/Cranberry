#pragma once

#include "../../Resources/IVulkanResources.h"
#include "../../../RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "../VulkanMacros.h"

class VulkanShaderSetParamsLayout : public ShaderSetParametersLayout, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderSetParamsLayout,,ShaderSetParametersLayout,)
public:
    VkDescriptorSetLayout descriptorLayout;

private:
    std::vector<VkDescriptorPoolSize> poolAllocation;
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
protected:
    VulkanShaderSetParamsLayout() = default;
public:
    VulkanShaderSetParamsLayout(const ShaderResource* shaderResource, uint32 setID);

    /* IVulkanResources overrides */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const final;

    /* ShaderParametersLayout overrides */
    void init() final;
    void release() final;
    String getResourceName() const final;

    /* GraphicsResource overrides */
    //void reinitResources() final;

    /* Override ends */

    const std::vector<VkDescriptorPoolSize>& getDescPoolAllocInfo() const;
};

// TODO(Jeslas) : change the way param info retrieved to something better and extend able later

// descriptor set layout and its info unique to each shader
class VulkanShaderUniqDescLayout final : public VulkanShaderSetParamsLayout
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderUniqDescLayout, , VulkanShaderSetParamsLayout, )
public:
    constexpr static uint32 DESC_SET_ID = 2;
private:
    VulkanShaderUniqDescLayout() = default;
public:
    VulkanShaderUniqDescLayout(const ShaderResource* shaderResource);

    /* VulkanShaderParamsLayout overrides */
    String getObjectName() const final;

    /* ShaderParametersLayout overrides */
protected:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const final;
    /* Override ends */
};

// This will be unique for each vertex type instance
class VulkanVertexUniqDescLayout final : public VulkanShaderSetParamsLayout
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanVertexUniqDescLayout, , VulkanShaderSetParamsLayout, )
public:
    constexpr static uint32 DESC_SET_ID = 1;
private:
    VulkanVertexUniqDescLayout() = default;
public:
    VulkanVertexUniqDescLayout(const ShaderResource* shaderResource);

    /* VulkanShaderParamsLayout overrides */
    String getObjectName() const final;

    /* ShaderParametersLayout overrides */
protected:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const final;
    /* Override ends */
};

// This will be unique for view scene
class VulkanViewUniqDescLayout final : public VulkanShaderSetParamsLayout
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanViewUniqDescLayout, , VulkanShaderSetParamsLayout, )
public:
    constexpr static uint32 DESC_SET_ID = 0;
private:
    VulkanViewUniqDescLayout() = default;
public:
    VulkanViewUniqDescLayout(const ShaderResource* shaderResource);

    /* VulkanShaderParamsLayout overrides */
    String getObjectName() const final;

    /* ShaderParametersLayout overrides */
protected:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const final;
    /* Override ends */
};

// For shaders other than DrawMeshShader
class VulkanShaderParametersLayout final : public ShaderParametersLayout, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderParametersLayout,,ShaderParametersLayout,)

private:
    // Holds a Set's parameters layout info
    struct SetParametersLayoutInfo
    {
        std::vector<VkDescriptorPoolSize> poolAllocation;
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

        VkDescriptorSetLayout descriptorLayout = nullptr;
    };
public:
    std::map<uint32, SetParametersLayoutInfo> setToLayoutInfo;

private:
    VulkanShaderParametersLayout() = default;
public:
    VulkanShaderParametersLayout(const ShaderResource* shaderResource);

    /* IVulkanResources overrides */
    String getObjectName() const final;

    /* ShaderParametersLayout overrides */
    void init() final;
    void release() final;
    String getResourceName() const final;

    /* GraphicsResource overrides */
    //void reinitResources() final;

    /* Override ends */

    const std::vector<VkDescriptorPoolSize>& getDescPoolAllocInfo(uint32 setIdx) const;
    VkDescriptorSetLayout getDescSetLayout(uint32 setIdx) const;
};

// For shaders and layouts of DrawMeshShaders
class VulkanShaderSetParameters final : public ShaderParameters, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderSetParameters, , ShaderParameters, )
private:
    struct DescriptorWriteData
    {
        uint32 writeInfoIdx;
        union
        {
            const TexelParameterData* texel;
            const TextureParameterData* texture;
            const SamplerParameterData* sampler;
        } ParamData;
    };
public:
    VkDescriptorSet descriptorsSet;

private:
    VulkanShaderSetParameters() = default;
public:
    VulkanShaderSetParameters(const GraphicsResource * shaderParamLayout)
        : BaseType(shaderParamLayout)
    {}

    /* IVulkanResources overrides */
    String getObjectName() const final;
    uint64 getDispatchableHandle() const final;
    /* ShaderParameters overrides */
    void init() final;
    void release() final;
    void updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance) final;
    /* Override ends */
};

// For shaders and layouts not corresponding to DrawMeshShaders
class VulkanShaderParameters final : public ShaderParameters, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderParameters, , ShaderParameters, )
private:
    struct DescriptorWriteData
    {
        uint32 setID;
        uint32 writeInfoIdx;
        union
        {
            const TexelParameterData* texel;
            const TextureParameterData* texture;
            const SamplerParameterData* sampler;
        } ParamData;
    };
public:
    std::map<uint32, VkDescriptorSet> descriptorsSets;
private:
    VulkanShaderParameters() = default;
public:
    VulkanShaderParameters(const GraphicsResource* shaderParamLayout, const std::set<uint32>& ignoredSetIds)
        : BaseType(shaderParamLayout, ignoredSetIds)
    {}

    /* IVulkanResources overrides */
    String getObjectName() const final;
    /* ShaderParameters overrides */
    void init() final;
    void release() final;
    void updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance) final;
    /* Override ends */
};