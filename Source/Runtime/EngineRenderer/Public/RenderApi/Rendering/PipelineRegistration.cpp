/*!
 * \file PipelineRegistration.cpp
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/Rendering/PipelineRegistration.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Resources/ShaderResources.h"

//////////////////////////////////////////////////////////////////////////
// PipelineFactory
//////////////////////////////////////////////////////////////////////////

GraphicsPipelineFactoryRegistrant::GraphicsPipelineFactoryRegistrant(const TChar *shaderName, GraphicsPipelineConfigGetter configGetter)
    : getter(configGetter)
{
    PipelineFactory::graphicsPipelineFactoriesRegistry().insert({ shaderName, *this });
}

FORCE_INLINE PipelineBase *GraphicsPipelineFactoryRegistrant::operator() (
    IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, const PipelineFactoryArgs &args
) const
{
    PipelineBase *pipeline;
    if (args.parentPipeline != nullptr)
    {
        pipeline = graphicsHelper->createGraphicsPipeline(graphicsInstance, args.parentPipeline);
    }
    else
    {
        fatalAssertf(getter.isBound(), "Invalid GraphicsPipelineConfig getter for shader {}", args.pipelineShader->getResourceName().getChar());
        String pipelineName;
        pipeline = graphicsHelper->createGraphicsPipeline(graphicsInstance, getter.invoke(pipelineName, args.pipelineShader));
        pipeline->setResourceName(pipelineName);
        pipeline->setPipelineShader(args.pipelineShader);
    }
    return pipeline;
}

ComputePipelineFactoryRegistrant::ComputePipelineFactoryRegistrant(const TChar *shaderName)
{
    PipelineFactory::computePipelineFactoriesRegistry().insert({ shaderName, *this });
}

FORCE_INLINE PipelineBase *ComputePipelineFactoryRegistrant::operator() (
    IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, const PipelineFactoryArgs &args
) const
{
    PipelineBase *pipeline;
    if (args.parentPipeline != nullptr)
    {
        pipeline = graphicsHelper->createComputePipeline(graphicsInstance, args.parentPipeline);
    }
    else
    {
        String pipelineName = TCHAR("Compute_") + args.pipelineShader->getResourceName();
        pipeline = graphicsHelper->createComputePipeline(graphicsInstance);
        pipeline->setResourceName(pipelineName);
        pipeline->setPipelineShader(args.pipelineShader);
    }
    return pipeline;
}

std::map<StringID, GraphicsPipelineFactoryRegistrant> &PipelineFactory::graphicsPipelineFactoriesRegistry()
{
    static std::map<StringID, GraphicsPipelineFactoryRegistrant> singletonPipelineFactoriesRegistry;
    return singletonPipelineFactoriesRegistry;
}

std::map<StringID, ComputePipelineFactoryRegistrant> &PipelineFactory::computePipelineFactoriesRegistry()
{
    static std::map<StringID, ComputePipelineFactoryRegistrant> singletonPipelineFactoriesRegistry;
    return singletonPipelineFactoriesRegistry;
}

PipelineBase *
PipelineFactory::create(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, const PipelineFactoryArgs &args) const
{
    fatalAssertf(args.pipelineShader, "Pipeline shader cannot be null");
    if (args.pipelineShader->getShaderConfig()->getType()->isChildOf<DrawMeshShaderConfig>()
        || args.pipelineShader->getShaderConfig()->getType()->isChildOf<UniqueUtilityShaderConfig>())
    {
        auto factoryItr = graphicsPipelineFactoriesRegistry().find(StringID(args.pipelineShader->getResourceName()));
        fatalAssertf(
            factoryItr != graphicsPipelineFactoriesRegistry().end(), "Failed finding factory to create graphics pipeline for shader {}",
            args.pipelineShader->getResourceName().getChar()
        );

        return (factoryItr->second)(graphicsInstance, graphicsHelper, args);
    }
    else if (args.pipelineShader->getShaderConfig()->getType()->isChildOf<ComputeShaderConfig>())
    {
        auto factoryItr = computePipelineFactoriesRegistry().find(StringID(args.pipelineShader->getResourceName()));
        fatalAssertf(
            factoryItr != computePipelineFactoriesRegistry().end(), "Failed finding factory to create compute pipeline for shader {}",
            args.pipelineShader->getResourceName().getChar()
        );

        return (factoryItr->second)(graphicsInstance, graphicsHelper, args);
    }
    LOG_ERROR("PipelineFactory", "Pipeline factory unsupported shader config/shader");
    return nullptr;
}