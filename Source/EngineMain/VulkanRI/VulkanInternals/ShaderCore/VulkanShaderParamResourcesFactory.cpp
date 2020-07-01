#include "VulkanShaderParamResourcesFactory.h"
#include "../../../RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "VulkanShaderParamResources.h"
#include "../../../Core/Logger/Logger.h"

GraphicsResource* VulkanShaderParametersLayoutFactory::create(const ShaderResource* forShader, uint32 descriptorsSetIdx) const
{
    if (forShader->getType()->isChildOf(DrawMeshShader::staticType()))
    {
        switch (descriptorsSetIdx)
        {
        case VulkanShaderUniqDescLayout::DESC_SET_ID:
            return new VulkanShaderUniqDescLayout(forShader);
        case VulkanVertexUniqDescLayout::DESC_SET_ID:
            return new VulkanVertexUniqDescLayout(forShader);
        case VulkanViewUniqDescLayout::DESC_SET_ID:
            return new VulkanViewUniqDescLayout(forShader);
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

