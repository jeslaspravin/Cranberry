#pragma once
#include "GraphicsResources.h"
#include "String/String.h"
#include "Types/Patterns/FactoriesBase.h"
#include "Types/CoreDefines.h"
#include "Types/Delegates/Delegate.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "EngineRendererExports.h"

#include <map>

class ShaderResource;
class GraphicsHelperAPI;
class IGraphicsInstance;

class ENGINERENDERER_EXPORT PipelineCacheBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(PipelineCacheBase, , GraphicsResource, )
private:
    String cacheName;
    String cacheFileName;
protected:
    std::vector<const class PipelineBase*> pipelinesToCache;

    // raw data of pipeline cache file to write out
    virtual std::vector<uint8> getRawToWrite() const { return {}; }
    std::vector<uint8> getRawFromFile() const;
public:
    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String& name) final;

    /* Override ends */

    void addPipelineToCache(const class PipelineBase* pipeline);
    void writeCache() const;
};

//////////////////////////////////////////////////////////////////////////
// Pipeline related codes
//////////////////////////////////////////////////////////////////////////


class ENGINERENDERER_EXPORT PipelineBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(PipelineBase, , GraphicsResource, )

private:
    String pipelineName;
protected:
    // If this pipeline will be used as parent to any other pipelines?
    bool bCanBeParent;

    const PipelineBase* parentPipeline;
    const PipelineCacheBase* parentCache;

    const ShaderResource* pipelineShader;
    std::vector<const GraphicsResource*> shaderParamLayouts;// At each set index changes based on mesh draw shader or others

protected:
    PipelineBase() = default;
    PipelineBase(const PipelineBase *parent);
public:

    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String& name) final;

    /* Override ends */

    void setParentPipeline(const PipelineBase* parent);
    void setPipelineShader(const ShaderResource* shader) { pipelineShader = shader; }
    void setParamLayoutAtSet(const GraphicsResource* paramLayout, int32 setIdx = -1);

    void setCanBeParent(bool bIsParent) { bCanBeParent = bIsParent; }
    void setPipelineCache(const PipelineCacheBase* pipelineCache);

    const GraphicsResource* getParamLayoutAtSet(int32 setIdx) const;
    const ShaderResource* getShaderResource() const { return pipelineShader; }
};

//////////////////////////////////////////////////////////////////////////
/// Graphics pipeline related types
//////////////////////////////////////////////////////////////////////////

namespace EPrimitiveTopology
{
    enum Type
    {
        Triangle,
        Line,
        Point
    };

    template<Type PrimType>
    constexpr const AChar* getChar()
    {
        switch (PrimType)
        {
        case EPrimitiveTopology::Triangle:
            return "Triangle";
        case EPrimitiveTopology::Line:
            return "Line";
            break;
        case EPrimitiveTopology::Point:
            return "Point";
        default:
            break;
        }
        return "";
    }
}

struct GraphicsPipelineQueryParams
{
    EPolygonDrawMode drawMode;
    ECullingMode cullingMode;
};

struct GraphicsPipelineConfig
{
    // In draw mesh shader, only renderpassAttachmentFormat of below property will be valid value
    GenericRenderPassProperties renderpassProps;

    EPrimitiveTopology::Type primitiveTopology = EPrimitiveTopology::Triangle;
    // Tessellation control points per patch, If zero ignored
    uint32 cntrlPts = 0;

    bool bEnableDepthBias = false;
    bool bEnableDepthClamp = false;

    DepthState depthState;
    StencilState stencilStateFront;
    StencilState stencilStateBack;

    std::vector<AttachmentBlendState> attachmentBlendStates;

    // Dynamic params(All permuted combination of pipeline with below states will be created)
    std::vector<EPolygonDrawMode> allowedDrawModes;
    std::vector<ECullingMode> supportedCullings;
};

class ENGINERENDERER_EXPORT GraphicsPipelineBase : public PipelineBase
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsPipelineBase,, PipelineBase,)

protected:
    GraphicsPipelineConfig config;
protected:
    GraphicsPipelineBase() = default;
    GraphicsPipelineBase(const GraphicsPipelineBase * parent);

    GraphicsPipelineQueryParams paramForIdx(int32 idx) const;
    int32 idxFromParam(GraphicsPipelineQueryParams queryParam) const;

    FORCE_INLINE int32 pipelinesCount() const
    {
        return int32(config.allowedDrawModes.size() * config.supportedCullings.size());
    }

public:

    void setPipelineConfig(const GraphicsPipelineConfig& newConfig) { config = newConfig; }
    void setRenderpassProperties(const GenericRenderPassProperties& newProps) { config.renderpassProps = newProps; }

    const GenericRenderPassProperties& getRenderpassProperties() const { return config.renderpassProps; }
};

//////////////////////////////////////////////////////////////////////////
/// Compute pipeline related types
//////////////////////////////////////////////////////////////////////////

class ENGINERENDERER_EXPORT ComputePipelineBase : public PipelineBase
{
    DECLARE_GRAPHICS_RESOURCE(ComputePipelineBase, , PipelineBase, )

protected:
    ComputePipelineBase() = default;
    ComputePipelineBase(const ComputePipelineBase* parent);

public:

};

//////////////////////////////////////////////////////////////////////////
// PipelineFactory
//////////////////////////////////////////////////////////////////////////

struct PipelineFactoryArgs
{
    const ShaderResource* pipelineShader;
    const PipelineBase* parentPipeline = nullptr;
};

/*
* Pipeline registration
*/
#define CREATE_GRAPHICS_PIPELINE_REGISTRANT(Registrant, ShaderName, FunctionPtr) \
    GraphicsPipelineFactoryRegistrant Registrant(ShaderName \
        , GraphicsPipelineFactoryRegistrant::GraphicsPipelineConfigGetter::createStatic(FunctionPtr))

struct ENGINERENDERER_EXPORT GraphicsPipelineFactoryRegistrant
{
    using GraphicsPipelineConfigGetter = SingleCastDelegate<GraphicsPipelineConfig, String&, const ShaderResource*>;
    GraphicsPipelineConfigGetter getter;

public:
    GraphicsPipelineFactoryRegistrant(const String& shaderName, GraphicsPipelineConfigGetter configGetter);
    FORCE_INLINE PipelineBase* operator()(IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper, const PipelineFactoryArgs& args) const;
};

struct ENGINERENDERER_EXPORT ComputePipelineFactoryRegistrant
{
public:
    ComputePipelineFactoryRegistrant(const String& shaderName);
    FORCE_INLINE PipelineBase* operator()(IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper, const PipelineFactoryArgs& args) const;
};

class ENGINERENDERER_EXPORT PipelineFactory final : public FactoriesBase<PipelineBase*, IGraphicsInstance*, const GraphicsHelperAPI*, const PipelineFactoryArgs&>
{
private:
    friend GraphicsPipelineFactoryRegistrant;
    friend ComputePipelineFactoryRegistrant;

    static std::map<String, GraphicsPipelineFactoryRegistrant>& graphicsPipelineFactoriesRegistry();
    static std::map<String, ComputePipelineFactoryRegistrant>& computePipelineFactoriesRegistry();
public:
    PipelineBase* create(IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper, const PipelineFactoryArgs& args) const final;
};