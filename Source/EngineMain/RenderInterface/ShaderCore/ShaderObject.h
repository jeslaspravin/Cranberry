#pragma once
#include "../../Core/String/String.h"
#include "../../RenderApi/VertexData.h"
#include "ShaderInputOutput.h"

#include <set>
#include <unordered_map>

class GraphicsResourceType;
class ShaderResource;
class DrawMeshShader;
class UniqueUtilityShader;
struct FramebufferFormat;
struct GenericRenderpassProperties;

class GraphicsPipeline;
using PGraphicsPipelineBase = GraphicsPipeline*;//Ptr
using PPGraphicsPipelineBaseRef = PGraphicsPipelineBase*;// Ptr to Ptr

class ShaderObjectBase
{
protected:
    String shaderName;
public:
    ShaderObjectBase(const String& sName);
    virtual ~ShaderObjectBase() = default;
};

/**
* DrawMeshShaderObject - Encapsulates a shader's related objects like all shader resource that are this shader
* but belonging to different vertex type and render passes
*
* @author Jeslas Pravin
*
* @date June 2020
*/
class DrawMeshShaderObject : ShaderObjectBase
{
private:
    using ShaderResourcePair = std::pair<const DrawMeshShader*, PGraphicsPipelineBase>;
    using ShaderResourceList = std::vector<ShaderResourcePair>;
    using ShaderResourcesIterator = ShaderResourceList::iterator;
    using ShaderResourcesConstIterator = ShaderResourceList::const_iterator;

private:
    ShaderResourceList shaderResources;

    std::unordered_map<EVertexType::Type, std::set<int32>> shadersForVertexType;
    std::unordered_map<FramebufferFormat, std::set<int32>> shadersForRenderPass;

public:
    DrawMeshShaderObject(const String& sName);

    const DrawMeshShader* getShader(EVertexType::Type inputVertexType, const FramebufferFormat& outputBufferFormat
        , PPGraphicsPipelineBaseRef outGraphicsPipeline = nullptr) const;

    // Internal use functions
    void addShader(const ShaderResource* shaderResource, const FramebufferFormat& usageFormats);
    void setPipeline(const ShaderResource* shaderResource, PGraphicsPipelineBase graphicsPipeline);
};


/**
* UniqueUtilityShaderObject - Encapsulates a single permutation shader and all the pipelines that are for this shader
* but corresponding to different render pass attachment format or multi sample rate
*
* @author Jeslas Pravin
*
* @date July 2020
*/
class UniqueUtilityShaderObject : ShaderObjectBase
{
private:
    const UniqueUtilityShader* utilityShader;

    GenericRenderpassProperties defaultPipelineProps;
    std::unordered_map<GenericRenderpassProperties, PGraphicsPipelineBase> graphicsPipelines;
public:
    UniqueUtilityShaderObject(const String& sName, const ShaderResource* shaderResource);

    const UniqueUtilityShader* getShader() const;
    PGraphicsPipelineBase getPipeline(const GenericRenderpassProperties& renderpassProps) const;
    PGraphicsPipelineBase getDefaultPipeline() const;

    // Internal use functions
    void setPipeline(const GenericRenderpassProperties& renderpassProps, PGraphicsPipelineBase graphicsPipeline);
};