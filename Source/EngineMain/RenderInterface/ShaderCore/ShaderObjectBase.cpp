#include "ShaderObject.h"
#include "../Shaders/Base/DrawMeshShader.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "../../Core/Math/Math.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../Resources/Pipelines.h"

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

const DrawMeshShader* DrawMeshShaderObject::getShader(EVertexType::Type inputVertexType, const FramebufferFormat& outputBufferFormat
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
                *outGraphicsPipeline = shaderResources[shaderResIndices[0]].second;
            }
            return shaderResources[shaderResIndices[0]].first;
        }
    }
    return nullptr;
}

void DrawMeshShaderObject::addShader(const ShaderResource* shaderResource)
{
    const DrawMeshShader* drawMeshShader = static_cast<const DrawMeshShader*>(shaderResource);
    FramebufferFormat usageFormats(drawMeshShader->renderpassUsage());
    ShaderResourcesConstIterator itr = std::find_if(shaderResources.cbegin(), shaderResources.cend()
        , [drawMeshShader](const ShaderResourcePair& shaderPipelinePair) { return shaderPipelinePair.first == drawMeshShader; });
    if (itr == shaderResources.cend())
    {
        int32 shaderResIndex = int32(shaderResources.size());
        shaderResources.emplace_back(ShaderResourcePair(drawMeshShader, nullptr));


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
    const DrawMeshShader* drawMeshShader = static_cast<const DrawMeshShader*>(shaderResource);
    ShaderResourcesIterator itr = std::find_if(shaderResources.begin(), shaderResources.end()
        , [drawMeshShader](const ShaderResourcePair& shaderPipelinePair) { return shaderPipelinePair.first == drawMeshShader; });

    debugAssert(itr != shaderResources.end());

    itr->second = graphicsPipeline;
}

DrawMeshShaderObject::~DrawMeshShaderObject()
{
    for (const ShaderResourcePair& shaderPipelinePair : shaderResources)
    {
        shaderPipelinePair.second->release();
        delete shaderPipelinePair.second;
    }
    shaderResources.clear();
}

const DrawMeshShaderObject::ShaderResourceList& DrawMeshShaderObject::getAllShaders() const
{
    return shaderResources;
}

void DrawMeshShaderObject::preparePipelineCache(PipelineCacheBase* pipelineCache) const
{
    for (const std::pair<const DrawMeshShader*, GraphicsPipelineBase*>& shaderResourcePair : getAllShaders())
    {
        pipelineCache->addPipelineToCache(shaderResourcePair.second);
    }
}

const GraphicsResourceType* DrawMeshShaderObject::baseShaderType() const
{
    return DrawMeshShader::staticType();
}

//////////////////////////////////////////////////////////////////////////
// UniqueUtilityShaderObject
//////////////////////////////////////////////////////////////////////////

UniqueUtilityShaderObject::UniqueUtilityShaderObject(const String& sName, const ShaderResource* shaderResource)
    : ShaderObjectBase(sName)
    , utilityShader(static_cast<const UniqueUtilityShader*>(shaderResource))
{}

const UniqueUtilityShader* UniqueUtilityShaderObject::getShader() const
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
    return UniqueUtilityShader::staticType();
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
    , computeShader(static_cast<const ComputeShader*>(shaderResource))
{}

ComputeShaderObject::~ComputeShaderObject()
{
    computePipeline->release();
    delete computePipeline;
}

const ComputeShader* ComputeShaderObject::getShader() const
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
    return ComputeShader::staticType();
}

void ComputeShaderObject::preparePipelineCache(PipelineCacheBase* pipelineCache) const
{
    pipelineCache->addPipelineToCache(computePipeline);
}