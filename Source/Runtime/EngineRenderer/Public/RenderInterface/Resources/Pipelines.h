/*!
 * \file Pipelines.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "GraphicsResources.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "String/String.h"
#include "EngineRendererExports.h"

class ShaderResource;
class GraphicsHelperAPI;
class IGraphicsInstance;
class PipelineBase;

class ENGINERENDERER_EXPORT PipelineCacheBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(PipelineCacheBase, , GraphicsResource, )
private:
    String cacheName;
    String cacheFileName;

protected:
    std::vector<const PipelineBase *> pipelinesToCache;

    // raw data of pipeline cache file to write out
    virtual std::vector<uint8> getRawToWrite() const { return {}; }
    std::vector<uint8> getRawFromFile() const;

public:
    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String &name) final;

    /* Override ends */

    void addPipelineToCache(const PipelineBase *pipeline);
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

    const PipelineBase *parentPipeline;
    const PipelineCacheBase *parentCache;

    const ShaderResource *pipelineShader;
    std::vector<const GraphicsResource *> shaderParamLayouts; // At each set index changes based on mesh draw shader or others

protected:
    PipelineBase() = default;
    PipelineBase(const PipelineBase *parent);

public:
    /* GraphicsResource overrides */
    String getResourceName() const final;
    void setResourceName(const String &name) final;

    /* Override ends */

    void setParentPipeline(const PipelineBase *parent);
    void setPipelineShader(const ShaderResource *shader) { pipelineShader = shader; }
    void setParamLayoutAtSet(const GraphicsResource *paramLayout, int32 setIdx = -1);

    void setCanBeParent(bool bIsParent) { bCanBeParent = bIsParent; }
    void setPipelineCache(const PipelineCacheBase *pipelineCache);

    const GraphicsResource *getParamLayoutAtSet(int32 setIdx) const;
    const ShaderResource *getShaderResource() const { return pipelineShader; }
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

template <Type PrimType>
constexpr const TChar *getChar()
{
    switch (PrimType)
    {
    case EPrimitiveTopology::Triangle:
        return TCHAR("Triangle");
    case EPrimitiveTopology::Line:
        return TCHAR("Line");
        break;
    case EPrimitiveTopology::Point:
        return TCHAR("Point");
    default:
        break;
    }
    return TCHAR("");
}
} // namespace EPrimitiveTopology

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
    DECLARE_GRAPHICS_RESOURCE(GraphicsPipelineBase, , PipelineBase, )

protected:
    GraphicsPipelineConfig config;

protected:
    GraphicsPipelineBase() = default;
    GraphicsPipelineBase(const GraphicsPipelineBase *parent);

    GraphicsPipelineQueryParams paramForIdx(int32 idx) const;
    int32 idxFromParam(GraphicsPipelineQueryParams queryParam) const;

    FORCE_INLINE int32 pipelinesCount() const { return int32(config.allowedDrawModes.size() * config.supportedCullings.size()); }

public:
    void setPipelineConfig(const GraphicsPipelineConfig &newConfig) { config = newConfig; }
    void setRenderpassProperties(const GenericRenderPassProperties &newProps) { config.renderpassProps = newProps; }

    const GenericRenderPassProperties &getRenderpassProperties() const { return config.renderpassProps; }
};

//////////////////////////////////////////////////////////////////////////
/// Compute pipeline related types
//////////////////////////////////////////////////////////////////////////

class ENGINERENDERER_EXPORT ComputePipelineBase : public PipelineBase
{
    DECLARE_GRAPHICS_RESOURCE(ComputePipelineBase, , PipelineBase, )

protected:
    ComputePipelineBase() = default;
    ComputePipelineBase(const ComputePipelineBase *parent);

public:
};