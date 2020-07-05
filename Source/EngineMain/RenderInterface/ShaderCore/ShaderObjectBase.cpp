#include "ShaderObject.h"
#include "../Rendering/FramebufferTypes.h"
#include "../Shaders/Base/DrawMeshShader.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "../../Core/Math/Math.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

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
    , PPGraphicsPipelineBaseRef outGraphicsPipeline /*= nullptr*/) const
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

void DrawMeshShaderObject::addShader(const ShaderResource* shaderResource, const FramebufferFormat& usageFormats)
{
    const DrawMeshShader* drawMeshShader = static_cast<const DrawMeshShader*>(shaderResource);
    ShaderResourcesConstIterator itr = std::find_if(shaderResources.cbegin(), shaderResources.cend()
        , [drawMeshShader](ShaderResourcesConstIterator& itr) { return itr->first == drawMeshShader; });
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

void DrawMeshShaderObject::setPipeline(const ShaderResource* shaderResource, PGraphicsPipelineBase graphicsPipeline)
{
    const DrawMeshShader* drawMeshShader = static_cast<const DrawMeshShader*>(shaderResource);
    ShaderResourcesIterator itr = std::find_if(shaderResources.begin(), shaderResources.end()
        , [drawMeshShader](ShaderResourcesIterator& itr) { return itr->first == drawMeshShader; });

    debugAssert(itr != shaderResources.end());

    itr->second = graphicsPipeline;
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

PGraphicsPipelineBase UniqueUtilityShaderObject::getPipeline(const GenericRenderpassProperties& renderpassProps) const
{
    std::unordered_map<GenericRenderpassProperties, PGraphicsPipelineBase>::const_iterator itr = graphicsPipelines.find(renderpassProps);

    return itr != graphicsPipelines.cend() ? itr->second : nullptr;
}

PGraphicsPipelineBase UniqueUtilityShaderObject::getDefaultPipeline() const
{
    return graphicsPipelines.find(defaultPipelineProps)->second;
}

void UniqueUtilityShaderObject::setPipeline(const GenericRenderpassProperties& renderpassProps, PGraphicsPipelineBase graphicsPipeline)
{
    if (graphicsPipelines.empty())
    {
        defaultPipelineProps = renderpassProps;
    }
    else
    {
        std::unordered_map<GenericRenderpassProperties, PGraphicsPipelineBase>::iterator itr = graphicsPipelines.find(renderpassProps);
        debugAssert(itr != graphicsPipelines.end());
    }
    graphicsPipelines[renderpassProps] = graphicsPipeline;
}
