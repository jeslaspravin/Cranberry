#include "VulkanShaderResources.h"
#include "../../Core/Engine/GameEngine.h"
#include "../VulkanGraphicsHelper.h"
#include "../VulkanInternals/Debugging.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderCodeResource, VK_OBJECT_TYPE_SHADER_MODULE)

void VulkanShaderCodeResource::reinitResources()
{
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    if (shaderModule)
    {
        VulkanGraphicsHelper::destroyShaderModule(graphicsInstance, shaderModule);
    }

    BaseType::reinitResources();
    shaderModule = VulkanGraphicsHelper::createShaderModule(graphicsInstance,shaderCode.data(), (uint32)shaderCode.size());
    if (shaderModule != nullptr)
    {
        VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(this);
    }
}

void VulkanShaderCodeResource::release()
{
    if (shaderModule)
    {
        VulkanGraphicsHelper::destroyShaderModule(gEngine->getRenderApi()->getGraphicsInstance(), shaderModule);
        shaderModule = nullptr;
    }
    BaseType::release();
}

String VulkanShaderCodeResource::getObjectName() const
{
    return getResourceName();
}

uint64 VulkanShaderCodeResource::getDispatchableHandle() const
{
    return (uint64)shaderModule;
}

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderResource, VK_OBJECT_TYPE_SHADER_MODULE)

ShaderCodeResource* VulkanShaderResource::createShaderCode(const String& filePath) const
{
    return new VulkanShaderCodeResource(filePath);
}

VulkanShaderResource::VulkanShaderResource(const String& name) : BaseType(name)
{
    if (!shaderCodeFactory)
    {
        shaderCodeFactory = static_cast<ShaderCodeFactory::ClassDelegate>(&VulkanShaderResource::createShaderCode);
    }
}

VulkanShaderResource::VulkanShaderResource() : BaseType()
{
    if (!shaderCodeFactory)
    {
        shaderCodeFactory = static_cast<ShaderCodeFactory::ClassDelegate>(&VulkanShaderResource::createShaderCode);
    }
}

String VulkanShaderResource::getObjectName() const
{
    return getResourceName();
}