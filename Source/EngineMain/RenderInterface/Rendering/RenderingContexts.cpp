#include "RenderingContexts.h"
#include "../Shaders/Base/DrawMeshShader.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "../ShaderCore/ShaderParameters.h"
#include "../PlatformIndependentHeaders.h"
#include "../ShaderCore/ShaderParameterUtility.h"

#include <set>

void GlobalRenderingContextBase::initContext(IGraphicsInstance* graphicsInstance)
{
    initApiFactories();

    std::set<EVertexType::Type> filledVertexInfo;
    auto vertexAttribFillLambda = [&filledVertexInfo](EVertexType::Type vertexUsed, const std::vector<ReflectInputOutput>& vertexShaderInputs)
    {
        // If not filled yet
        if (filledVertexInfo.find(vertexUsed) == filledVertexInfo.end())
        {
            filledVertexInfo.insert(vertexUsed);
            const std::vector<ShaderVertexParamInfo*>& vertexBindingsInfo = EVertexType::vertexParamInfo(vertexUsed);
            for (ShaderVertexParamInfo* vertexBindingAttributes : vertexBindingsInfo)
            {
                ShaderParameterUtility::fillRefToVertexParamInfo(*vertexBindingAttributes, vertexShaderInputs);
            }
        }
    };

    std::vector<GraphicsResource*> defaultModeShaders;
    GraphicsShaderResource::staticType()->allChildDefaultResources(defaultModeShaders);
    for (GraphicsResource* shader : defaultModeShaders)
    {
        shader->init();

        if (shader->getType()->isChildOf(DrawMeshShader::staticType()))
        {
            DrawMeshShader* drawMeshShader = static_cast<DrawMeshShader*>(shader);
            vertexAttribFillLambda(drawMeshShader->vertexUsage(), drawMeshShader->getReflection()->inputs);

            // TODO(Jeslas) : change below to init for all permutations under each material
        }
        else if (shader->getType()->isChildOf(UniqueUtilityShader::staticType()))
        {
            UniqueUtilityShader* utilityShader = static_cast<UniqueUtilityShader*>(shader);
            vertexAttribFillLambda(utilityShader->vertexUsage(), utilityShader->getReflection()->inputs);
        }
    }
}

void GlobalRenderingContextBase::clearContext()
{
    // TODO(Jeslas) : clear properly
    std::vector<GraphicsResource*> shaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(shaderResources, true);
    for (GraphicsResource* shader : shaderResources)
    {
        shader->release();
    }
}
