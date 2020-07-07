#pragma once
#include "GraphicsResources.h"
#include "../../Core/String/String.h"
#include "../CoreGraphicsTypes.h"
#include "../Rendering/FramebufferTypes.h"
#include "../../Core/Types/Patterns/FactoriesBase.h"

#include <map>

class ShaderResource;

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

    const ShaderResource* pipelineShader;
    std::vector<const GraphicsResource*> shaderParamLayouts;// At each set index changes based on mesh draw shader or others

protected:
    PipelineBase() = default;
    PipelineBase(const PipelineBase * parent);
public:

    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String& name) final;

    /* Override ends */

    void setParentPipeline(const PipelineBase* parent);
    void setPipelineShader(const ShaderResource* shader) { pipelineShader = shader; }
    void setParamLayoutAtSet(const GraphicsResource* paramLayout, int32 setIdx = -1);

    const GraphicsResource* getParamLayoutAtSet(int32 setIdx) const;
    const ShaderResource* getShaderResource() const { return pipelineShader; }
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

    EVertexTopology::Type primitiveTopology;
    // Tessellation control points per patch, If zero ignored
    uint32 cntrlPts = 0;
    std::vector<ECullingMode> supportedCullings;

    DepthState depthState;
    StencilState stencilState;
    std::vector<AttachmentBlendState> attachmentBlendStates;
protected:
    GraphicsPipeline() = default;
    GraphicsPipeline(const GraphicsPipeline * parent);
public:

    void setRenderpassProperties(const GenericRenderpassProperties& newProps) { renderpassProps = newProps; }

    const std::vector<ECullingMode>& getCullingModes() const { return supportedCullings; }
    const GenericRenderpassProperties& getRenderpassProperties() const { return renderpassProps; }
};


//////////////////////////////////////////////////////////////////////////
// PipelineFactory
//////////////////////////////////////////////////////////////////////////

struct PipelineFactoryArgs
{
    const ShaderResource* pipelineShader;
    const PipelineBase* parentPipeline = nullptr;
};

struct PipelineFactoryRegister
{
    PipelineFactoryRegister(const String& shaderName);
    virtual PipelineBase* operator()(const PipelineFactoryArgs& args) const = 0;
};

class PipelineFactory : public FactoriesBase<PipelineBase, const PipelineFactoryArgs&>
{
private:
    friend PipelineFactoryRegister;
    static std::map<String, const PipelineFactoryRegister*> REGISTERED_PIPELINE_FACTORIES;
public:
    PipelineBase* create(const PipelineFactoryArgs& args) const override;

};