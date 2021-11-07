#pragma once
#include "RenderApi/VertexData.h"
#include "Types/Patterns/FactoriesBase.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "EngineRendererExports.h"

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
struct Framebuffer;

/////////////////////////////////////////////////////////////////////////////////
// Contains most of the global common items that could be one time initialized
// This is API independent class
//
// Possible global context data
// Shader pipelines, Pipeline layouts, Descriptors set layouts,
// maybe some shader binding data, Common render passes
/////////////////////////////////////////////////////////////////////////////////
class ENGINERENDERER_EXPORT GlobalRenderingContextBase
{
private:
    friend class RenderManager;

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
    GraphicsResource* bindlessParamLayout = nullptr;

    std::unordered_map<GenericRenderPassProperties, std::vector<const Framebuffer*>> rtFramebuffers;
    PipelineCacheBase* pipelinesCache;

    FactoriesBase<ShaderObjectBase, const String&, const ShaderResource*>* shaderObjectFactory;
    FactoriesBase<GraphicsResource, const ShaderResource*, uint32>* shaderParamLayoutsFactory;
    FactoriesBase<PipelineBase, const PipelineFactoryArgs&>* pipelineFactory;

private:
    void initContext(IGraphicsInstance* graphicsInstance);
    void clearContext();

    void initShaderResources();
    void initShaderPipelines(const std::vector<GraphicsResource*>& allShaderResources
        , const std::map<String, std::pair<uint32, ShaderResource*>>& shaderUniqParamShader);
    void destroyShaderResources();
    void writeAndDestroyPipelineCache();
protected:
    // Graphics API specific codes
    virtual void initApiInstances() = 0;
    virtual void initializeApiContext() = 0;
    virtual void clearApiContext() = 0;

    // Functions for none mesh draw shader passes

    // Fills necessary render pass info to pipeline(Pipeline render pass properties has to filled before using this) and initializes it
    virtual void initializeGenericGraphicsPipeline(UniqueUtilityShaderObject* shaderObject, PipelineBase* pipeline) = 0;
    // Get generic render pass properties from Render targets
    GenericRenderPassProperties renderpassPropsFromRTs(const std::vector<RenderTargetTexture*>& rtTextures) const;
    // Get generic render pass properties from framebuffer
    GenericRenderPassProperties renderpassPropsFromFb(const Framebuffer* fb) const;
    // Get generic render pass properties from render pass format, Useful in case of using custom RTs with predefined render passes 
    GenericRenderPassProperties renderpassPropsFromRpFormat(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx) const;
    const Framebuffer* getFramebuffer(const GenericRenderPassProperties& renderpassProps
        , const std::vector<RenderTargetTexture*>& rtTextures) const;
    const Framebuffer* createNewFramebuffer(const GenericRenderPassProperties& renderpassProps
        , const std::vector<RenderTargetTexture*>& rtTextures) const;
    const Framebuffer* getOrCreateFramebuffer(const GenericRenderPassProperties& renderpassProps
        , const std::vector<RenderTargetTexture*>& rtTextures);
    // Creates new pipeline based on default pipeline of shader object but with new render pass or different render pass and returns it
    PipelineBase* createNewPipeline(UniqueUtilityShaderObject* shaderObject, const GenericRenderPassProperties& renderpassProps);

public:

    void preparePipelineContext(class LocalPipelineContext* pipelineContext);
    void clearExternInitRtsFramebuffer(const std::vector<RenderTargetTexture*>& rtTextures);
};

// Temporary class high chance to change later so avoid relying on this.
class ENGINERENDERER_EXPORT LocalPipelineContext
{
private:
    friend GlobalRenderingContextBase;

    const Framebuffer* framebuffer = nullptr;
    const PipelineBase* pipelineUsed = nullptr;

public:
    uint32 swapchainIdx;

    std::vector<RenderTargetTexture*> rtTextures;
    ERenderPassFormat::Type renderpassFormat;
    bool bUseSwapchainFb = false;

    EVertexType::Type forVertexType;

    String materialName;

    const Framebuffer* getFb() const { return framebuffer; }
    const PipelineBase* getPipeline() const { return pipelineUsed; }
};

struct ENGINERENDERER_EXPORT TinyDrawingContext
{
    const GraphicsResource* cmdBuffer;

    std::vector<RenderTargetTexture*> rtTextures;
    uint32 swapchainIdx = ~(0u);
};