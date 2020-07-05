#pragma once
#include "../../RenderApi/VertexData.h"
#include "../../Core/Types/Patterns/FactoriesBase.h"

#include <unordered_map>

class String;
class IGraphicsInstance;
class GraphicsResource;
class ShaderResource;
class ShaderObjectBase;
class RenderTargetTexture;
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

    struct ShaderDataCollection
    {
        ShaderObjectBase* shaderObject;

        // One for each unique of material(not shader)
        GraphicsResource* shadersParamLayout;
    };

    // Shader(material as all shader with same name considered as same material) name to collection
    std::unordered_map<String, ShaderDataCollection> rawShaderObjects;

    // Has one layout(Descriptors set layout) for each vertex type
    std::unordered_map<EVertexType::Type, GraphicsResource*> perVertexTypeLayouts;
    // Scene's common layout(Descriptors set layout)
    GraphicsResource* sceneViewParamLayout;

    std::unordered_map<GenericRenderpassProperties, std::vector<const Framebuffer*>> rtFramebuffers;

protected:

    FactoriesBase<ShaderObjectBase, const String&, const ShaderResource*>* shaderObjectFactory;
    FactoriesBase<GraphicsResource, const ShaderResource*, uint32>* shaderParamLayoutsFactory;

private:
    void initContext(IGraphicsInstance* graphicsInstance);
    void clearContext();

protected:
    virtual void initApiFactories() = 0;

    // Functions for none mesh draw shader passes
    // Get generic render pass properties from Render targets
    GenericRenderpassProperties renderpassPropsFromRts(const std::vector<RenderTargetTexture*>& rtTextures) const;
    const Framebuffer* getFramebuffer(const GenericRenderpassProperties& renderpassProps, const std::vector<RenderTargetTexture*>& rtTextures) const;
};