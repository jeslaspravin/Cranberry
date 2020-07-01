#include "GraphicsPipeline.h"

DEFINE_GRAPHICS_RESOURCE(GraphicsPipelineCacheBase)

String GraphicsPipelineCacheBase::getResourceName() const
{
    return cacheName;
}
void GraphicsPipelineCacheBase::setResourceName(const String& name)
{
    cacheName = name;
}

//////////////////////////////////////////////////////////////////////////
// Pipeline resource
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(GraphicsPipelineBase)

String GraphicsPipelineBase::getResourceName() const
{
    return pipelineName;
}

void GraphicsPipelineBase::setResourceName(const String& name)
{
    pipelineName = name;
}
    