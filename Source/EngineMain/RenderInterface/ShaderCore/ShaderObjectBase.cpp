#include "ShaderObject.h"
#include "../Rendering/FramebufferTypes.h"
#include "../Resources/ShaderResources.h"
#include "../../Core/Math/Math.h"

#include <algorithm>

MeshDrawShaderObjectBase::MeshDrawShaderObjectBase(const String& sName)
    : shaderName(sName)
{}

const ShaderResource* MeshDrawShaderObjectBase::getShader(EVertexType::Type inputVertexType, const FramebufferFormat& outputBufferFormat) const
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
            return shaderResources[shaderResIndices[0]];
        }
    }
    return nullptr;
}

void MeshDrawShaderObjectBase::addShader(const ShaderResource* shaderResource, const FramebufferFormat& usageFormats)
{
    ShaderResourcesConstIterator itr = std::find(shaderResources.cbegin(),shaderResources.cend(), shaderResource);
    if (itr == shaderResources.cend())
    {
        int32 shaderResIndex = int32(shaderResources.size());
        shaderResources.emplace_back(shaderResource);


        auto shadersForFormatItr = shadersForRenderPass.find(usageFormats);
        if (shadersForFormatItr == shadersForRenderPass.end())
        {
            shadersForRenderPass[usageFormats] = { shaderResIndex };
        }
        else
        {
            shadersForFormatItr->second.insert(shaderResIndex);
        }

        auto shadersForVertex = shadersForVertexType.find(shaderResource->vertexUsage());
        if (shadersForVertex == shadersForVertexType.end())
        {
            shadersForVertexType[shaderResource->vertexUsage()] = { shaderResIndex };
        }
        else
        {
            shadersForVertex->second.insert(shaderResIndex);
        }
    }
}
