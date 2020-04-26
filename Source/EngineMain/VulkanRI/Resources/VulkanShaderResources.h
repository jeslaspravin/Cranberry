#pragma once
#include "../../RenderInterface/Resources/ShaderResources.h"
#include "IVulkanResources.h"
#include "../VulkanInternals/VulkanMacros.h"

class VulkanShaderCodeResource final : public ShaderCodeResource, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderCodeResource,,ShaderCodeResource,)
public:
    VkShaderModule shaderModule;
protected:
    VulkanShaderCodeResource(): BaseType(), shaderModule(nullptr){}
public:
    VulkanShaderCodeResource(const String& filePath)
        : BaseType(filePath)
        , shaderModule(nullptr)
    {}
    /* IVulkanResources overrides */
    String getObjectName() const override;
    void setObjectName(const String& name) override {}
    uint64 getDispatchableHandle() const override;
    /* ShaderResource overrides */
    void reinitResources() override;
    void release() override;
    /* End overrides */
};

class VulkanShaderResource : public ShaderResource, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderResource,,ShaderResource,)
private:
    ShaderCodeResource* createShaderCode(const String& filePath) const;
    VulkanShaderResource();
protected:
    VulkanShaderResource(const String& name);
public:
    /* IVulkanResources overrides */
    String getObjectName() const override;
    void setObjectName(const String& name) override {}
    uint64 getDispatchableHandle() const override { return 0; }
    /* End overrides */
};

namespace GraphicsTypes
{
    typedef VulkanShaderResource GraphicsShaderResource;
}