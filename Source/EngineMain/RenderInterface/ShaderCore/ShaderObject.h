#pragma once
#include "../../Core/String/String.h"
#include "../../RenderApi/VertexData.h"
#include "../Rendering/FramebufferTypes.h"
#include "ShaderInputOutput.h"

#include <set>
#include <unordered_map>

class GraphicsResourceType;
class ShaderResource;
class DrawMeshShader;
class UniqueUtilityShader;
struct FramebufferFormat;

class GraphicsPipelineBase;

class ShaderObjectBase
{
private:
    String shaderName;

protected:
    ShaderObjectBase(const String& sName);
public:
    virtual ~ShaderObjectBase() = default;

    const String& getShaderName() const { return shaderName; }

    virtual const GraphicsResourceType* baseShaderType() const = 0;
};

/**
* DrawMeshShaderObject - Encapsulates a shader's related objects like all shader resource that are this shader
* but belonging to different vertex type and render passes
*
* @author Jeslas Pravin
*
* @date June 2020
*/
class DrawMeshShaderObject final : public ShaderObjectBase
{
private:
    using ShaderResourcePair = std::pair<const DrawMeshShader*, GraphicsPipelineBase*>;
    using ShaderResourceList = std::vector<ShaderResourcePair>;
    using ShaderResourcesIterator = ShaderResourceList::iterator;
    using ShaderResourcesConstIterator = ShaderResourceList::const_iterator;

private:
    ShaderResourceList shaderResources;

    std::unordered_map<EVertexType::Type, std::set<int32>> shadersForVertexType;
    std::unordered_map<FramebufferFormat, std::set<int32>> shadersForRenderPass;

public:
    DrawMeshShaderObject(const String& sName);
    ~DrawMeshShaderObject();

    const DrawMeshShader* getShader(EVertexType::Type inputVertexType, const FramebufferFormat& outputBufferFormat
        ,  GraphicsPipelineBase** outGraphicsPipeline = nullptr) const;
    const ShaderResourceList& getAllShaders() const;

    // Internal use functions
    void addShader(const ShaderResource* shaderResource);
    void setPipeline(const ShaderResource* shaderResource, GraphicsPipelineBase* graphicsPipeline);

    /* ShaderObjectBase overrides */
    const GraphicsResourceType* baseShaderType() const final;

    /* Override ends */
};


/**
* UniqueUtilityShaderObject - Encapsulates a single permutation shader and all the pipelines that are for this shader
* but corresponding to different render pass attachment format or multi sample rate
*
* @author Jeslas Pravin
*
* @date July 2020
*/
class UniqueUtilityShaderObject final : public ShaderObjectBase
{
private:
    const UniqueUtilityShader* utilityShader;

    GenericRenderPassProperties defaultPipelineProps;
    std::unordered_map<GenericRenderPassProperties, GraphicsPipelineBase*> graphicsPipelines;
public:
    UniqueUtilityShaderObject(const String& sName, const ShaderResource* shaderResource);
    ~UniqueUtilityShaderObject();

    const UniqueUtilityShader* getShader() const;
    GraphicsPipelineBase* getPipeline(const GenericRenderPassProperties& renderpassProps) const;
    GraphicsPipelineBase* getDefaultPipeline() const;
    std::vector<const GraphicsPipelineBase*> getAllPipelines() const;

    // Internal use functions
    void setPipeline(const GenericRenderPassProperties& renderpassProps, GraphicsPipelineBase* graphicsPipeline);

    /* ShaderObjectBase overrides */
    const GraphicsResourceType* baseShaderType() const final;

    /* Override ends */
};