/*!
 * \file RenderingContexts.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "EngineRendererExports.h"
#include "RenderApi/VertexData.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "Types/Patterns/FactoriesBase.h"

#include <unordered_map>

class IGraphicsInstance;
class GraphicsResource;
class ShaderResource;
class ShaderObjectBase;
class UniqueUtilityShaderObject;
class PipelineBase;
class PipelineCacheBase;
struct PipelineFactoryArgs;
struct Framebuffer;
class GraphicsHelperAPI;
class GenericWindowCanvas;
class LocalPipelineContext;

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
        ShaderObjectBase *shaderObject = nullptr;

        // One for each unique of material(not shader)
        GraphicsResource *shadersParamLayout = nullptr;
    };
    IGraphicsInstance *graphicsInstanceCache;
    const GraphicsHelperAPI *graphicsHelperCache;

    // Shader(material as all shader with same name considered as same material) name to collection
    std::unordered_map<StringID, ShaderDataCollection> rawShaderObjects;

    // Has one layout(Descriptors set layout) for each vertex type
    std::unordered_map<EVertexType::Type, GraphicsResource *> perVertexTypeLayouts;
    // Scene's common layout(Descriptors set layout)
    GraphicsResource *sceneViewParamLayout = nullptr;
    GraphicsResource *bindlessParamLayout = nullptr;

    std::unordered_map<GenericRenderPassProperties, std::vector<const Framebuffer *>> rtFramebuffers;
    PipelineCacheBase *pipelinesCache;

    // One for each swapchain
    std::unordered_map<WindowCanvasRef, std::vector<const Framebuffer *>> windowCanvasFramebuffers;

    FactoriesBase<ShaderObjectBase *, const String &, const ShaderResource *> *shaderObjectFactory;
    FactoriesBase<GraphicsResource *, const ShaderResource *, uint32> *shaderParamLayoutsFactory;
    FactoriesBase<PipelineBase *, IGraphicsInstance *, const GraphicsHelperAPI *, const PipelineFactoryArgs &> *pipelineFactory;

private:
    void initContext(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void clearContext();

    void initShaderResources();
    void initShaderPipelines(
        const std::vector<ShaderResource *> &allShaderResources,
        const std::map<StringID, std::pair<uint32, ShaderResource *>> &shaderUniqParamShader
    );
    void destroyShaderResources();
    void writeAndDestroyPipelineCache();

protected:
    // Graphics API specific codes
    virtual void initApiInstances() = 0;
    virtual void initializeApiContext() = 0;
    virtual void clearApiContext() = 0;

    // Functions for none mesh draw shader passes

    // Fills necessary render pass info to pipeline(Pipeline render pass properties has to filled before
    // using this) and initializes it
    virtual void initializeGenericGraphicsPipeline(UniqueUtilityShaderObject *shaderObject, PipelineBase *pipeline) = 0;
    // Get generic render pass properties from Render targets, Moved to RenderManager.h
    // GenericRenderPassProperties renderpassPropsFromRTs(const std::vector<RenderTargetTexture*>&
    // rtTextures) const;
    // Get generic render pass properties from framebuffer
    GenericRenderPassProperties renderpassPropsFromFb(const Framebuffer *fb) const;
    // Get generic render pass properties from render pass format, Useful in case of using custom RTs
    // with predefined render passes
    GenericRenderPassProperties renderpassPropsFromRpFormat(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx) const;
    const Framebuffer *
        getFramebuffer(const GenericRenderPassProperties &renderpassProps, const std::vector<ImageResourceRef> &frameAttachments) const;
    const Framebuffer *
        createNewFramebuffer(const GenericRenderPassProperties &renderpassProps, const std::vector<ImageResourceRef> &frameAttachments) const;
    const Framebuffer *
        getOrCreateFramebuffer(const GenericRenderPassProperties &renderpassProps, const std::vector<ImageResourceRef> &frameAttachments);
    // Creates new pipeline based on default pipeline of shader object but with new render pass or
    // different render pass and returns it
    PipelineBase *createNewPipeline(UniqueUtilityShaderObject *shaderObject, const GenericRenderPassProperties &renderpassProps);

public:
    void preparePipelineContext(LocalPipelineContext *pipelineContext, GenericRenderPassProperties renderpassProps);
    void clearExternInitRtsFramebuffer(const std::vector<ImageResourceRef> &frameAttachments, GenericRenderPassProperties renderpassProps);
    void clearWindowCanvasFramebuffer(WindowCanvasRef windowCanvas);
};
