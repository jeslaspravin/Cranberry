#pragma once
#include "RenderApi/VertexData.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"

struct GraphicsPipelineConfig;

#define DEFAULT_SHADER_NAME "Default"

class ENGINERENDERER_EXPORT DrawMeshShaderConfig : public ShaderConfigCollector
{
    DECLARE_GRAPHICS_RESOURCE(DrawMeshShaderConfig,, ShaderConfigCollector,)
protected:
    EVertexType::Type compatibleVertex;
    ERenderPassFormat::Type compatibleRenderpassFormat;

private:
    DrawMeshShaderConfig() = default;
protected:
    DrawMeshShaderConfig(const String& name) 
        : BaseType(name)
    {}

    /* ShaderResource overrides */
    String getShaderFileName() const final;
    virtual void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const override;
public:
    EVertexType::Type vertexUsage() const { return compatibleVertex; }
    ERenderPassFormat::Type renderpassUsage() const { return compatibleRenderpassFormat; }
};

namespace CommonGraphicsPipelineConfigs
{
    GraphicsPipelineConfig writeGbufferShaderConfig(String& pipelineName, const ShaderResource* shaderResource);
}