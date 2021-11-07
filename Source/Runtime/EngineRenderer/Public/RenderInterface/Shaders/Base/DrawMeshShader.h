#pragma once
#include "RenderApi/VertexData.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "RenderInterface/PlatformIndependentHeaders.h"

#define DEFAULT_SHADER_NAME "Default"

class DrawMeshShader : public GraphicsShaderResource
{
    DECLARE_GRAPHICS_RESOURCE(DrawMeshShader,, GraphicsShaderResource,)
protected:
    EVertexType::Type compatibleVertex;
    ERenderPassFormat::Type compatibleRenderpassFormat;

private:
    DrawMeshShader() = default;
protected:
    DrawMeshShader(const String& name) : BaseType(name){}

    /* ShaderResource overrides */
    String getShaderFileName() const final;
    virtual void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const override;
public:
    EVertexType::Type vertexUsage() const { return compatibleVertex; }
    ERenderPassFormat::Type renderpassUsage() const { return compatibleRenderpassFormat; }
};