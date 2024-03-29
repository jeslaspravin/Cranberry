/*!
 * \file DrawMeshShader.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderApi/VertexData.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "RenderInterface/Resources/ShaderResources.h"

struct GraphicsPipelineConfig;

#define DEFAULT_SHADER_NAME TCHAR("Default")

class ENGINERENDERER_EXPORT DrawMeshShaderConfig : public ShaderConfigCollector
{
    DECLARE_GRAPHICS_RESOURCE(DrawMeshShaderConfig, , ShaderConfigCollector, )
protected:
    EVertexType::Type compatibleVertex;
    ERenderPassFormat::Type compatibleRenderpassFormat;

private:
    DrawMeshShaderConfig() = default;

protected:
    DrawMeshShaderConfig(const String &name)
        : BaseType(name)
    {}

    /* ShaderResource overrides */
    String getShaderFileName() const final;
    virtual void getSpecializationConsts(SpecConstantNamedMap &specializationConst) const override;

public:
    EVertexType::Type vertexUsage() const { return compatibleVertex; }
    ERenderPassFormat::Type renderpassUsage() const { return compatibleRenderpassFormat; }
};

namespace CommonGraphicsPipelineConfigs
{
GraphicsPipelineConfig writeGbufferShaderConfig(String &pipelineName, const ShaderResource *shaderResource);
}