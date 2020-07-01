#include "VulkanRenderingContexts.h"
#include "../ShaderCore/VulkanShaderParamResourcesFactory.h"

void VulkanGlobalRenderingContext::initApiFactories()
{
    shaderParamLayoutsFactory = new VulkanShaderParametersLayoutFactory();
}
