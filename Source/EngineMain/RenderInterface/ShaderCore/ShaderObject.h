#pragma once
#include "../../Core/String/String.h"
#include "../../RenderApi/VertexData.h"
#include "ShaderInputOutput.h"

#include <map>
#include <set>

class ShaderResource;
class GraphicsResourceType;
struct FramebufferFormat;

/**
* ShaderObject - Encapsulates a shader's related objects like all shader resource that are this shader
* but belonging to different vertex type and render passes
*
* @author Jeslas Pravin
*
* @date June 2020
*/
class MeshDrawShaderObjectBase
{
private:
    using ShaderResourceList = std::vector<const ShaderResource*>;
    using ShaderResourcesIterator = ShaderResourceList::iterator;
    using ShaderResourcesConstIterator = ShaderResourceList::const_iterator;

private:
    String shaderName;
    ShaderResourceList shaderResources;

    std::map<EVertexType::Type, std::set<int32>> shadersForVertexType;
    std::map<FramebufferFormat, std::set<int32>> shadersForRenderPass;

public:

    MeshDrawShaderObjectBase(const String& sName);

    const ShaderResource* getShader(EVertexType::Type inputVertexType, const FramebufferFormat& outputBufferFormat) const;

    // Internal use functions
    void addShader(const ShaderResource* shaderResource, const FramebufferFormat& usageFormats);
};