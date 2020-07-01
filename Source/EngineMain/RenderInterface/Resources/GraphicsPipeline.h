#pragma once
#include "GraphicsResources.h"
#include "../../Core/String/String.h"

class GraphicsPipelineCacheBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsPipelineCacheBase, , GraphicsResource, )

    String cacheName;

public:
    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String& name) final;

    /* Override ends */
};


class GraphicsPipelineBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsPipelineBase,, GraphicsResource,)

    String pipelineName;

public:
    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String& name) final;

    /* Override ends */
};
