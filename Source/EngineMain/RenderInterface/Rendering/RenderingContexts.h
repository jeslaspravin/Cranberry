#pragma once
#include "../../RenderApi/VertexData.h"
#include "../../Core/Types/Patterns/FactoriesBase.h"

#include <unordered_map>

class String;
class IGraphicsInstance;
class GraphicsResource;
class ShaderResource;
class ShaderObjectBase;
class UniqueUtilityShaderObject;
class PipelineBase;
class PipelineCacheBase;
class RenderTargetTexture;
struct PipelineFactoryArgs;
struct GenericRenderpassProperties;
struct Framebuffer;

/////////////////////////////////////////////////////////////////////////////////
// Contains most of the global common items that could be one time initialized
// This is API independent class
//
// Possible global context data
// Shader pipelines, Pipeline layouts, Descriptors set layouts,
// maybe some shader binding data, Common render passes
/////////////////////////////////////////////////////////////////////////////////
class GlobalRenderingContextBase
{
private:
    friend class RenderApi;

protected:

    struct ShaderDataCollection
    {
        ShaderObjectBase* shaderObject = nullptr;

        // One for each unique of material(not shader)
        GraphicsResource* shadersParamLayout = nullptr;
    };

    // Shader(material as all shader with same name considered as same material) name to collection
    std::unordered_map<String, ShaderDataCollection> rawShaderObjects;

    // Has one layout(Descriptors set layout) for each vertex type
    std::unordered_map<EVertexType::Type, GraphicsResource*> perVertexTypeLayouts;
    // Scene's common layout(Descriptors set layout)
    GraphicsResource* sceneViewParamLayout = nullptr;

    std::unordered_map<GenericRenderpassProperties, std::vector<const Framebuffer*>> rtFramebuffers;
    PipelineCacheBase* pipelinesCache;

    FactoriesBase<ShaderObjectBase, const String&, const ShaderResource*>* shaderObjectFactory;
    FactoriesBase<GraphicsResource, const ShaderResource*, uint32>* shaderParamLayoutsFactory;
    FactoriesBase<PipelineBase, const PipelineFactoryArgs&>* pipelineFactory;

private:
    void initContext(IGraphicsInstance* graphicsInstance);
    void clearContext();

    void initShaderResources();
    void destroyShaderResources();
protected:
    const GraphicsResource* getSceneViewParamLayout() const { return sceneViewParamLayout; }
    const GraphicsResource* getVertexTypeParamLayout(EVertexType::Type vertexType) const { return perVertexTypeLayouts.at(vertexType); }

    // Graphics API specific codes
    virtual void initApiFactories() = 0;
    virtual void initializeApiContext() = 0;
    virtual void clearApiContext() = 0;

    // Functions for none mesh draw shader passes

    // Fills necessary values to pipeline and initializes it
    virtual void initializeNewPipeline(UniqueUtilityShaderObject* shaderObject, PipelineBase* pipeline) = 0;
    // Get generic render pass properties from Render targets
    GenericRenderpassProperties renderpassPropsFromRTs(const std::vector<RenderTargetTexture*>& rtTextures) const;
    const Framebuffer* getFramebuffer(const GenericRenderpassProperties& renderpassProps
        , const std::vector<RenderTargetTexture*>& rtTextures) const;
    const Framebuffer* createNewFramebuffer(const GenericRenderpassProperties& renderpassProps
        , const std::vector<RenderTargetTexture*>& rtTextures) const;
    // Creates new pipeline based on default pipeline of shader object but with new render pass or different render pass and returns it
    PipelineBase* createNewPipeline(UniqueUtilityShaderObject* shaderObject, const GenericRenderpassProperties& renderpassProps);

};