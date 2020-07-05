#include "GraphicsPipeline.h"
#include "ShaderResources.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

DEFINE_GRAPHICS_RESOURCE(PipelineCacheBase)

String PipelineCacheBase::getResourceName() const
{
    return cacheName;
}
void PipelineCacheBase::setResourceName(const String& name)
{
    cacheName = name;
}

PipelineBase::PipelineBase(const PipelineBase* parent)
    : pipelineName(parent->pipelineName)
    , parentPipeline(parent)
    , parentPipelineIdx(-1)
    , pipelineShader(parent->pipelineShader)
    , shaderParamLayouts(parent->shaderParamLayouts)
{}

//////////////////////////////////////////////////////////////////////////
// Pipeline resource
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(PipelineBase)

String PipelineBase::getResourceName() const
{
    return pipelineName;
}

void PipelineBase::setResourceName(const String& name)
{
    pipelineName = name;
}
    

DEFINE_GRAPHICS_RESOURCE(GraphicsPipeline)

GraphicsPipeline::GraphicsPipeline(const GraphicsPipeline* parent)
    : PipelineBase(parent)
    , renderpassProps(parent->renderpassProps)
    , primitiveTopology(parent->primitiveTopology)
    , cntrlPts(parent->cntrlPts)
    , cullingMode(parent->cullingMode)
    , multisampling(parent->multisampling)
    , depthState(parent->depthState)
    , stencilState(parent->stencilState)
    , attachmentBlendStates(parent->attachmentBlendStates)
{}

//////////////////////////////////////////////////////////////////////////
// PipelineFactory
//////////////////////////////////////////////////////////////////////////

PipelineFactoryRegister::PipelineFactoryRegister(const String& shaderName)
{
    PipelineFactory::REGISTERED_PIPELINE_FACTORIES.insert({ shaderName , this });
}

std::map<String, const PipelineFactoryRegister*> PipelineFactory::REGISTERED_PIPELINE_FACTORIES;

PipelineBase* PipelineFactory::create(const PipelineFactoryArgs& args) const
{
    auto factoryItr = REGISTERED_PIPELINE_FACTORIES.find(args.pipelineShader->getResourceName());
    fatalAssert(factoryItr != REGISTERED_PIPELINE_FACTORIES.end(), "Failed finding factory to create pipeline for shader");

    return (*factoryItr->second)(args);
}
