#pragma once
#include "../../RenderApi/VertexData.h"
#include "../../Core/Types/Patterns/FactoriesBase.h"

#include <map>

class MeshDrawShaderObjectBase;
class String;
class IGraphicsInstance;
class GraphicsResource;
class ShaderResource;

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
        // For mesh draw shaders this will be not null
        MeshDrawShaderObjectBase* meshDrawShaders;
        // For unique shaders like utility or computes this will be not null
        ShaderResource* shader;

        // One for each unique of material(not shader)
        GraphicsResource* shadersParamLayout;
    };

    // Shader(material as all shader with same name considered as same material) name to collection
    std::map<String, ShaderDataCollection> rawShaderObjects;

    // Has one layout(Descriptors set layout) for each vertex type
    std::map<EVertexType::Type, GraphicsResource*> perVertexTypeLayouts;
    // Scene's common layout(Descriptors set layout)
    GraphicsResource* sceneViewParamLayout;

protected:

    FactoriesBase<MeshDrawShaderObjectBase, const String&>* meshShaderObjectFactory;
    FactoriesBase<GraphicsResource, const ShaderResource*, uint32>* shaderParamLayoutsFactory;

private:
    void initContext(IGraphicsInstance* graphicsInstance);
    void clearContext();

protected:
    virtual void initApiFactories() = 0;
};