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
    const std::vector<VkDescriptorSetLayoutBinding>& getDescSetBindings() const;
};

// descriptor set layout and its info unique to each shader
class VulkanShaderUniqDescLayout final : public VulkanShaderSetParamsLayout
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderUniqDescLayout, , VulkanShaderSetParamsLayout, )
private:
    VulkanShaderUniqDescLayout() = default;
public:
    VulkanShaderUniqDescLayout(const ShaderResource* shaderResource, uint32 descSetIdx);

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

// Bindless global descriptor set, Right now this does not have any buffers so not need to bind any buffer param info
class VulkanBindlessDescLayout final : public VulkanShaderSetParamsLayout
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanBindlessDescLayout, , VulkanShaderSetParamsLayout, )
private:
    VulkanBindlessDescLayout() = default;
public:
    VulkanBindlessDescLayout(const ShaderResource* shaderResource);

    /* VulkanShaderParamsLayout overrides */
    String getObjectName() const final;

    /* ShaderParametersLayout overrides */
protected:
    // void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const final;
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
        bool bHasBindless = false;

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
    bool hasBindless(uint32 setIdx) const final;

    /* GraphicsResource overrides */
    //void reinitResources() final;

    /* Override ends */

    const std::vector<VkDescriptorPoolSize>& getDescPoolAllocInfo(uint32 setIdx) const;
    const std::vector<VkDescriptorSetLayoutBinding>& getDescSetBindings(uint32 setIdx) const;
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
        uint32 arrayIdx = 0;
        union
        {
            const BufferParametersData* buffer;
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
        uint32 arrayIdx = 0;
        union
        {
            const BufferParametersData* buffer;
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