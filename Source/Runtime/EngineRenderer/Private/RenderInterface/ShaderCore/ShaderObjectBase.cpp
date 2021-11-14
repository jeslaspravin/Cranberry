#include "RenderInterface/ShaderCore/ShaderObject.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/Resources/Pipelines.h"

#include <algorithm>


ShaderObjectBase::ShaderObjectBase(const String& sName)
    : shaderName(sName)
{}

//////////////////////////////////////////////////////////////////////////
// MeshDrawShaderObject
//////////////////////////////////////////////////////////////////////////

DrawMeshShaderObject::DrawMeshShaderObject(const String& sName)
    : ShaderObjectBase(sName)
{}

const ShaderResource* DrawMeshShaderObject::getShader(EVertexType::Type inputVertexType, const FramebufferFormat& outputBufferFormat
    , GraphicsPipelineBase** outGraphicsPipeline /*= nullptr*/) const
{
    auto shadersForFormatItr = shadersForRenderPass.find(outputBufferFormat);
    auto shadersForVertex = shadersForVertexType.find(inputVertexType);
    if (shadersForFormatItr != shadersForRenderPass.cend() && shadersForVertex != shadersForVertexType.cend())
    {
        std::vector<int32> shaderResIndices(Math::min(shadersForFormatItr->second.size(), shadersForVertex->second.size()));
        std::vector<int32>::iterator shaderResIndicesEnd = std::set_intersection(shadersForFormatItr->second.cbegin()
            , shadersForFormatItr->second.cend(), shadersForVertex->second.cbegin(), shadersForVertex->second.cend(), shaderResIndices.begin());

        if ((shaderResIndicesEnd - shaderResIndices.begin()) > 0)
        {
            if (outGraphicsPipeline)
            {
                *outGraphicsPipeline = shaderResources[shaderResIndices[0]].pipeline;
            }
            return shaderResources[shaderResIndices[0]].shader;
        }
    }
    return nullptr;
}

GraphicsResource* DrawMeshShaderObject::getVariantUniqueParamsLayout(EVertexType::Type inputVertexType, const FramebufferFormat& outputBufferFormat) const
{
    auto shadersForFormatItr = shadersForRenderPass.find(outputBufferFormat);
    auto shadersForVertex = shadersForVertexType.find(inputVertexType);
    if (shadersForFormatItr != shadersForRenderPass.cend() && shadersForVertex != shadersForVertexType.cend())
    {
        std::vector<int32> shaderResIndices(Math::min(shadersForFormatItr->second.size(), shadersForVertex->second.size()));
        std::vector<int32>::iterator shaderResIndicesEnd = std::set_intersection(shadersForFormatItr->second.cbegin()
            , shadersForFormatItr->second.cend(), shadersForVertex->second.cbegin(), shadersForVertex->second.cend(), shaderResIndices.begin());

        if ((shaderResIndicesEnd - shaderResIndices.begin()) > 0)
        {
            return shaderResources[shaderResIndices[0]].perVariantParamsLayout;
        }
    }
    return nullptr;
}

void DrawMeshShaderObject::addShader(const ShaderResource* shaderResource)
{
    const DrawMeshShaderConfig* drawMeshShader = static_cast<const DrawMeshShaderConfig*>(shaderResource->getShaderConfig());
    FramebufferFormat usageFormats(drawMeshShader->renderpassUsage());
    ShaderResourcesConstIterator itr = std::find_if(shaderResources.cbegin(), shaderResources.cend()
        , [shaderResource](const ShaderResourceInfo& shaderPipelineInfo) { return shaderPipelineInfo.shader == shaderResource; });
    if (itr == shaderResources.cend())
    {
        int32 shaderResIndex = int32(shaderResources.size());
        shaderResources.emplace_back(ShaderResourceInfo{ shaderResource, nullptr, nullptr });

        auto shadersForFormatItr = shadersForRenderPass.find(usageFormats);
        if (shadersForFormatItr == shadersForRenderPass.end())
        {
            shadersForRenderPass[usageFormats] = { shaderResIndex };
        }
        else
        {
            shadersForFormatItr->second.insert(shaderResIndex);
        }

        auto shadersForVertex = shadersForVertexType.find(drawMeshShader->vertexUsage());
        if (shadersForVertex == shadersForVertexType.end())
        {
            shadersForVertexType[drawMeshShader->vertexUsage()] = { shaderResIndex };
        }
        else
        {
            shadersForVertex->second.insert(shaderResIndex);
        }
    }
}

void DrawMeshShaderObject::setPipeline(const ShaderResource* shaderResource, GraphicsPipelineBase* graphicsPipeline)
{
    ShaderResourcesIterator itr = std::find_if(shaderResources.begin(), shaderResources.end()
        , [shaderResource](const ShaderResourceInfo& shaderPipelinePair) { return shaderPipelinePair.shader == shaderResource; });

    debugAssert(itr != shaderResources.end());

    itr->pipeline = graphicsPipeline;
}

void DrawMeshShaderObject::setVariantParamsLayout(const ShaderResource* shaderResource, GraphicsResource* perVariantParamsLayout)
{
    ShaderResourcesIterator itr = std::find_if(shaderResources.begin(), shaderResources.end()
        , [shaderResource](const ShaderResourceInfo& shaderPipelinePair) { return shaderPipelinePair.shader == shaderResource; });

    debugAssert(itr != shaderResources.end());

    itr->perVariantParamsLayout = perVariantParamsLayout;
}

DrawMeshShaderObject::~DrawMeshShaderObject()
{
    for (const ShaderResourceInfo& shaderPipelinePair : shaderResources)
    {
        shaderPipelinePair.pipeline->release();
        delete shaderPipelinePair.pipeline;

        if(shaderPipelinePair.perVariantParamsLayout)
        {
            shaderPipelinePair.perVariantParamsLayout->release();
            delete shaderPipelinePair.perVariantParamsLayout;
        }
    }
    shaderResources.clear();
}

const DrawMeshShaderObject::ShaderResourceList& DrawMeshShaderObject::getAllShaders() const
{
    return shaderResources;
}

void DrawMeshShaderObject::preparePipelineCache(PipelineCacheBase* pipelineCache) const
{
    for (const ShaderResourceInfo& shaderResourcePair : getAllShaders())
    {
        pipelineCache->addPipelineToCache(shaderResourcePair.pipeline);
    }
}

const GraphicsResourceType* DrawMeshShaderObject::baseShaderType() const
{
    return DrawMeshShaderConfig::staticType();
}

//////////////////////////////////////////////////////////////////////////
// UniqueUtilityShaderObject
//////////////////////////////////////////////////////////////////////////

UniqueUtilityShaderObject::UniqueUtilityShaderObject(const String& sName, const ShaderResource* shaderResource)
    : ShaderObjectBase(sName)
    , utilityShader(shaderResource)
{}

const ShaderResource* UniqueUtilityShaderObject::getShader() const
{
    return utilityShader;
}

GraphicsPipelineBase* UniqueUtilityShaderObject::getPipeline(const GenericRenderPassProperties& renderpassProps) const
{
    std::unordered_map<GenericRenderPassProperties, GraphicsPipelineBase*>::const_iterator itr = graphicsPipelines.find(renderpassProps);

    return itr != graphicsPipelines.cend() ? itr->second : nullptr;
}

GraphicsPipelineBase* UniqueUtilityShaderObject::getDefaultPipeline() const
{
    return graphicsPipelines.find(defaultPipelineProps)->second;
}

void UniqueUtilityShaderObject::setPipeline(const GenericRenderPassProperties& renderpassProps, GraphicsPipelineBase* graphicsPipeline)
{
    if (graphicsPipelines.empty())
    {
        defaultPipelineProps = renderpassProps;
    }
    else
    {
        std::unordered_map<GenericRenderPassProperties, GraphicsPipelineBase*>::iterator itr = graphicsPipelines.find(renderpassProps);
        debugAssert(itr == graphicsPipelines.end());
    }
    graphicsPipelines[renderpassProps] = graphicsPipeline;
}

UniqueUtilityShaderObject::~UniqueUtilityShaderObject()
{
    for (const std::pair<const GenericRenderPassProperties, GraphicsPipelineBase*>& pipeline : graphicsPipelines)
    {
        pipeline.second->release();
        delete pipeline.second;
    }
    graphicsPipelines.clear();
}

const GraphicsResourceType* UniqueUtilityShaderObject::baseShaderType() const
{
    return UniqueUtilityShaderConfig::staticType();
}

void UniqueUtilityShaderObject::preparePipelineCache(PipelineCacheBase* pipelineCache) const
{
    for (const std::pair<const GenericRenderPassProperties, GraphicsPipelineBase*>& pipeline : graphicsPipelines)
    {
        pipelineCache->addPipelineToCache(pipeline.second);
    }
}

std::vector<const GraphicsPipelineBase*> UniqueUtilityShaderObject::getAllPipelines() const
{
    std::vector<const GraphicsPipelineBase*> pipelines;
    pipelines.reserve(graphicsPipelines.size());
    for (const std::pair<const GenericRenderPassProperties, GraphicsPipelineBase*>& pipeline : graphicsPipelines)
    {
        pipelines.emplace_back(pipeline.second);
    }
    return pipelines;
}

//////////////////////////////////////////////////////////////////////////
// ComputeShaderObject
//////////////////////////////////////////////////////////////////////////

ComputeShaderObject::ComputeShaderObject(const String& sName, const ShaderResource* shaderResource)
    : ShaderObjectBase(sName)
    , computeShader(shaderResource)
{}

ComputeShaderObject::~ComputeShaderObject()
{
    computePipeline->release();
    delete computePipeline;
}

const ShaderResource* ComputeShaderObject::getShader() const
{
    return computeShader;
}

ComputePipelineBase* ComputeShaderObject::getPipeline() const
{
    return computePipeline;
}

void ComputeShaderObject::setPipeline(ComputePipelineBase* pipeline)
{
    computePipeline = pipeline;
}

const GraphicsResourceType* ComputeShaderObject::baseShaderType() const
{
    return ComputeShaderConfig::staticType();
}

void ComputeShaderObject::preparePipelineCache(PipelineCacheBase* pipelineCache) const
{
    pipelineCache->addPipelineToCache(computePipeline);
}