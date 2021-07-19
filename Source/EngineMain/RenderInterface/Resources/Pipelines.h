#pragma once
#include "GraphicsResources.h"
#include "../../Core/String/String.h"
#include "../../Core/Types/CoreDefines.h"
#include "../CoreGraphicsTypes.h"
#include "../Rendering/FramebufferTypes.h"
#include "../../Core/Types/Patterns/FactoriesBase.h"

#include <map>

class ShaderResource;

class PipelineCacheBase : public GraphicsResource
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


class PipelineBase : public GraphicsResource
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

    uint32 apiInputAssemblyState(EPrimitiveTopology::Type inputAssembly);

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

class GraphicsPipelineBase : public PipelineBase
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsPipelineBase,, PipelineBase,)

protected:
    // In draw mesh shader, only renderpassAttachmentFormat of below property will be valid value
    GenericRenderPassProperties renderpassProps;

    EPrimitiveTopology::Type primitiveTopology = EPrimitiveTopology::Triangle;
    // Tessellation control points per patch, If zero ignored
    uint32 cntrlPts = 0;

    //bool bEnableDepthBias = false;

    DepthState depthState;
    StencilState stencilStateFront;
    StencilState stencilStateBack;

    std::vector<AttachmentBlendState> attachmentBlendStates;

    // Dynamic params(All permuted combination of pipeline with below states will be created)
    std::vector<EPolygonDrawMode> allowedDrawModes;
    std::vector<ECullingMode> supportedCullings;

protected:
    GraphicsPipelineBase() = default;
    GraphicsPipelineBase(const GraphicsPipelineBase * parent);

    GraphicsPipelineQueryParams paramForIdx(int32 idx) const;
    int32 idxFromParam(GraphicsPipelineQueryParams queryParam) const;
    FORCE_INLINE int32 pipelinesCount() const;

public:

    void setRenderpassProperties(const GenericRenderPassProperties& newProps) { renderpassProps = newProps; }

    const GenericRenderPassProperties& getRenderpassProperties() const { return renderpassProps; }
};

//////////////////////////////////////////////////////////////////////////
/// Compute pipeline related types
//////////////////////////////////////////////////////////////////////////

class ComputePipelineBase : public PipelineBase
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

struct PipelineFactoryRegistrar
{
    PipelineFactoryRegistrar(const String& shaderName);
    virtual PipelineBase* operator()(const PipelineFactoryArgs& args) const = 0;
};

/*
* Pipeline registration - common case
*/
template <typename PipelineType>
struct GenericPipelineRegistrar final : public PipelineFactoryRegistrar
{
    GenericPipelineRegistrar(const String& shaderName)
        : PipelineFactoryRegistrar(shaderName)
    {}

    PipelineBase* operator()(const PipelineFactoryArgs& args) const final
    {
        if (args.parentPipeline != nullptr)
        {
            return new PipelineType(args.parentPipeline);
        }
        else
        {
            return new PipelineType(args.pipelineShader);
        }
    }
};

class PipelineFactory final : public FactoriesBase<PipelineBase, const PipelineFactoryArgs&>
{
private:
    friend PipelineFactoryRegistrar;
    static std::map<String, const PipelineFactoryRegistrar*>& namedPipelineFactoriesRegistry();
public:
    PipelineBase* create(const PipelineFactoryArgs& args) const final;

};