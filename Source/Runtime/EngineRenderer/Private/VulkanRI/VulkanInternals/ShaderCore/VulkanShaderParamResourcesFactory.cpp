#include "VulkanRI/VulkanInternals/ShaderCore/VulkanShaderParamResourcesFactory.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "VulkanRI/VulkanInternals/ShaderCore/VulkanShaderParamResources.h"
#include "Logger/Logger.h"

GraphicsResource* VulkanShaderParametersLayoutFactory::create(const ShaderResource* forShader, uint32 descriptorsSetIdx) const
{
    if (forShader->getType()->isChildOf(DrawMeshShader::staticType()))
    {
        switch (descriptorsSetIdx)
        {
        case ShaderParameterUtility::INSTANCE_UNIQ_SET:
            return new VulkanVertexUniqDescLayout(forShader);
        case ShaderParameterUtility::VIEW_UNIQ_SET:
            return new VulkanViewUniqDescLayout(forShader);
        case ShaderParameterUtility::BINDLESS_SET:
            return new VulkanBindlessDescLayout(forShader);
        case ShaderParameterUtility::SHADER_UNIQ_SET:
        case ShaderParameterUtility::SHADER_VARIANT_UNIQ_SET:
            return new VulkanShaderUniqDescLayout(forShader, descriptorsSetIdx);
        default:
            Logger::error("VulkanShaderParametersLayoutFactory", "%s : Not support descriptor index %d for shader %s"
                , __func__, descriptorsSetIdx, forShader->getResourceName().getChar());
            return nullptr;
        }
    }
    else
    {
        return new VulkanShaderParametersLayout(forShader);
    }
}

