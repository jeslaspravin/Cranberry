#include "Pipelines.h"
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

//////////////////////////////////////////////////////////////////////////
// Pipeline resource
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(PipelineBase)

PipelineBase::PipelineBase(const PipelineBase* parent)
    : pipelineName(parent->pipelineName)
    , parentPipeline(parent)
    , pipelineShader(parent->pipelineShader)
    , shaderParamLayouts(parent->shaderParamLayouts)
{}

String PipelineBase::getResourceName() const
{
    return pipelineName;
}

void PipelineBase::setResourceName(const String& name)
{
    pipelineName = name;
}

void PipelineBase::setParentPipeline(const PipelineBase* parent)
{
    parentPipeline = parent;
}

void PipelineBase::setParamLayoutAtSet(const GraphicsResource* paramLayout, int32 setIdx /*= -1*/)
{
    if (setIdx < 0)
    {
        shaderParamLayouts.clear();
        shaderParamLayouts.emplace_back(paramLayout);
    }
    else
    {
        if (shaderParamLayouts.size() <= setIdx)
        {
            shaderParamLayouts.resize(setIdx + 1);
        }
        shaderParamLayouts[setIdx] = paramLayout;
    }
}

const GraphicsResource* PipelineBase::getParamLayoutAtSet(int32 setIdx) const
{
    return shaderParamLayouts[setIdx];
}

DEFINE_GRAPHICS_RESOURCE(GraphicsPipeline)

GraphicsPipeline::GraphicsPipeline(const GraphicsPipeline* parent)
    : PipelineBase(parent)
    , renderpassProps(parent->renderpassProps)
    , primitiveTopology(parent->primitiveTopology)
    , cntrlPts(parent->cntrlPts)
    , supportedCullings(parent->supportedCullings)
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
