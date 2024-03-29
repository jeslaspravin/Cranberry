/*!
 * \file ShaderObject.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <set>
#include <unordered_map>

#include "EngineRendererExports.h"
#include "RenderApi/VertexData.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "RenderInterface/ShaderCore/ShaderInputOutput.h"
#include "String/String.h"

class GraphicsResourceType;
class ShaderResource;
class DrawMeshShaderConfig;
class UniqueUtilityShaderConfig;
class ComputeShaderConfig;
struct FramebufferFormat;

class GraphicsPipelineBase;
class ComputePipelineBase;
class PipelineCacheBase;
class GraphicsResource;

class ENGINERENDERER_EXPORT ShaderObjectBase
{
private:
    String shaderName;

protected:
    ShaderObjectBase(const String &sName);

public:
    virtual ~ShaderObjectBase() = default;

    const String &getShaderName() const { return shaderName; }

    virtual const GraphicsResourceType *baseShaderType() const = 0;
    virtual void preparePipelineCache(PipelineCacheBase *pipelineCache) const = 0;
};

/**
 * DrawMeshShaderObject - Encapsulates a shader's related objects like all shader resource that are this
 * shader but belonging to different vertex type and render passes
 *
 * @author Jeslas Pravin
 *
 * @date June 2020
 */
class ENGINERENDERER_EXPORT DrawMeshShaderObject final : public ShaderObjectBase
{
public:
    struct ShaderResourceInfo
    {
        const ShaderResource *shader;
        GraphicsPipelineBase *pipeline;
        // In set 3
        GraphicsResource *perVariantParamsLayout;
    };
    using ShaderResourceList = std::vector<ShaderResourceInfo>;

private:
    using ShaderResourcesIterator = ShaderResourceList::iterator;
    using ShaderResourcesConstIterator = ShaderResourceList::const_iterator;

private:
    ShaderResourceList shaderResources;

    std::unordered_map<EVertexType::Type, std::set<int32>> shadersForVertexType;
    std::unordered_map<FramebufferFormat, std::set<int32>> shadersForRenderPass;

public:
    DrawMeshShaderObject(const String &sName);
    ~DrawMeshShaderObject();

    const ShaderResource *getShader(
        EVertexType::Type inputVertexType, const FramebufferFormat &outputBufferFormat, GraphicsPipelineBase **outGraphicsPipeline = nullptr
    ) const;
    GraphicsResource *getVariantUniqueParamsLayout(EVertexType::Type inputVertexType, const FramebufferFormat &outputBufferFormat) const;
    const ShaderResourceList &getAllShaders() const;

    // Internal use functions
    void addShader(const ShaderResource *shaderResource);
    void setPipeline(const ShaderResource *shaderResource, GraphicsPipelineBase *graphicsPipeline);
    void setVariantParamsLayout(const ShaderResource *shaderResource, GraphicsResource *perVariantParamsLayout);

    /* ShaderObjectBase overrides */
    const GraphicsResourceType *baseShaderType() const final;
    void preparePipelineCache(PipelineCacheBase *pipelineCache) const final;

    /* Override ends */
};

/**
 * UniqueUtilityShaderObject - Encapsulates a single permutation shader and all the pipelines that are
 * for this shader but corresponding to different render pass attachment format or multi sample rate
 *
 * @author Jeslas Pravin
 *
 * @date July 2020
 */
class ENGINERENDERER_EXPORT UniqueUtilityShaderObject final : public ShaderObjectBase
{
private:
    const ShaderResource *utilityShader;

    GenericRenderPassProperties defaultPipelineProps;
    std::unordered_map<GenericRenderPassProperties, GraphicsPipelineBase *> graphicsPipelines;

public:
    UniqueUtilityShaderObject(const String &sName, const ShaderResource *shaderResource);
    ~UniqueUtilityShaderObject();

    const ShaderResource *getShader() const;
    GraphicsPipelineBase *getPipeline(const GenericRenderPassProperties &renderpassProps) const;
    GraphicsPipelineBase *getDefaultPipeline() const;
    std::vector<const GraphicsPipelineBase *> getAllPipelines() const;

    // Internal use functions
    void setPipeline(const GenericRenderPassProperties &renderpassProps, GraphicsPipelineBase *graphicsPipeline);

    /* ShaderObjectBase overrides */
    const GraphicsResourceType *baseShaderType() const final;
    void preparePipelineCache(PipelineCacheBase *pipelineCache) const final;

    /* Override ends */
};

class ENGINERENDERER_EXPORT ComputeShaderObject final : public ShaderObjectBase
{
private:
    const ShaderResource *computeShader;
    ComputePipelineBase *computePipeline;

public:
    ComputeShaderObject(const String &sName, const ShaderResource *shaderResource);
    ~ComputeShaderObject();

    const ShaderResource *getShader() const;
    ComputePipelineBase *getPipeline() const;

    // Internal use functions
    void setPipeline(ComputePipelineBase *pipeline);

    /* ShaderObjectBase overrides */
    const GraphicsResourceType *baseShaderType() const final;
    void preparePipelineCache(PipelineCacheBase *pipelineCache) const final;

    /* Override ends */
};