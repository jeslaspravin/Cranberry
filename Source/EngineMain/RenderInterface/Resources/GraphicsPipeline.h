#pragma once
#include "GraphicsResources.h"
#include "../../Core/String/String.h"
#include "../CoreGraphicsTypes.h"
#include "../Rendering/FramebufferTypes.h"
#include "../../Core/Types/Patterns/FactoriesBase.h"

#include <map>

class ShaderResource;
class ShaderParametersLayout;

class PipelineCacheBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(PipelineCacheBase, , GraphicsResource, )

    String cacheName;

public:
    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String& name) final;

    /* Override ends */
};

//////////////////////////////////////////////////////////////////////////
// Pipeline related codes
//////////////////////////////////////////////////////////////////////////


class PipelineBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(PipelineBase, , GraphicsResource, )

private:
    String pipelineName;
protected:
    const PipelineBase* parentPipeline;
    int32 parentPipelineIdx = -1;// If both parent and this is being created in same create call.

    ShaderResource* pipelineShader;
    std::vector<ShaderParametersLayout*> shaderParamLayouts;// At each set index changes based on mesh draw shader or others

protected:
    PipelineBase() = default;
public:
    PipelineBase(const PipelineBase* parent);

    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String& name) final;

    /* Override ends */
};

namespace EVertexTopology
{
    enum Type
    {
        Triangle,
        Line,
        Point
    };

    uint32 apiInputAssemblyState(EVertexTopology::Type inputAssembly);
}

/*
* Right now no specialization constants are supported
*/
class GraphicsPipeline : public PipelineBase
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsPipeline,, PipelineBase,)

protected:
    // In draw mesh shader, only renderpassAttachmentFormat of below property will be valid value
    GenericRenderpassProperties renderpassProps;

    // TODO(Jeslas) Remove this Multi sampling from global value for gbuffers and from set value for other shaders

    EVertexTopology::Type primitiveTopology;
    // Tessellation control points per patch, If zero ignored
    uint32 cntrlPts = 0;

    ECullingMode cullingMode;
    // Considered from this only for non mesh pipelines
    EPixelSampleCount::Type multisampling;

    DepthState depthState;
    StencilState stencilState;
    std::vector<AttachmentBlendState> attachmentBlendStates;
protected:
    GraphicsPipeline() = default;
public:
    GraphicsPipeline(const GraphicsPipeline* parent);
};


//////////////////////////////////////////////////////////////////////////
// PipelineFactory
//////////////////////////////////////////////////////////////////////////

struct PipelineFactoryArgs
{
    ShaderResource* pipelineShader;
};

struct PipelineFactoryRegister
{
    PipelineFactoryRegister(const String& shaderName);
    virtual PipelineBase* operator()(const PipelineFactoryArgs& args) const = 0;
};

class PipelineFactory : FactoriesBase<PipelineBase, const PipelineFactoryArgs&>
{
private:
    friend PipelineFactoryRegister;
    static std::map<String, const PipelineFactoryRegister*> REGISTERED_PIPELINE_FACTORIES;
public:
    PipelineBase* create(const PipelineFactoryArgs& args) const override;

};