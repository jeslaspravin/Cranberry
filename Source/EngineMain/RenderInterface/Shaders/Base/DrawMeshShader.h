#pragma once
#include "../../../RenderApi/VertexData.h"
#include "../../Rendering/FramebufferTypes.h"
#include "../../PlatformIndependentHeaders.h"


class DrawMeshShader : public GraphicsShaderResource
{
    DECLARE_GRAPHICS_RESOURCE(DrawMeshShader,, GraphicsShaderResource,)
protected:
    EVertexType::Type compatibleVertex;
    ERenderpassFormat::Type compatibleRenderpassFormat;

private:
    DrawMeshShader() = default;
protected:
    DrawMeshShader(const String& name) : BaseType(name){}

    /* ShaderResource overrides */
    String getShaderFileName() const final;
public:
    EVertexType::Type vertexUsage() const { return compatibleVertex; }
    ERenderpassFormat::Type renderpassUsage() const { return compatibleRenderpassFormat; }
};

DEFINE_GRAPHICS_RESOURCE(DrawMeshShader)

String DrawMeshShader::getShaderFileName() const
{
    return getResourceName() + EVertexType::toString(vertexUsage()) + ERenderpassFormat::toString(renderpassUsage());
}
