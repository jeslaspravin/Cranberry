/*!
 * \file DrawingSimple3D.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderApi/Scene/RenderScene.h"
#include "RenderApi/Material/MaterialCommonUniforms.h"

#define DRAW_3D_COLORED_PER_VERTEX_NAME "Draw3DColoredPerVertex"
#define DRAW_3D_COLORED_PER_INSTANCE_NAME "Draw3DColoredPerInstance"
#define DIRECT_DRAW_3D_COLORED_PER_VERTEX_NAME "DirectDraw3DColoredPerVertex"
#define DIRECT_DRAW_3D_COLORED_PER_INSTANCE_NAME "DirectDraw3DColoredPerInstance"

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

template <EPrimitiveTopology::Type Topology, bool DepthWrite>
GraphicsPipelineConfig drawSimple3DPipelineConfig(String& pipelineName, const ShaderResource* shaderResource)
{
    pipelineName = "DrawSimple3D_" + shaderResource->getResourceName();
    GraphicsPipelineConfig config;

    config.supportedCullings.emplace_back(ECullingMode::BackFace);

    config.allowedDrawModes.emplace_back(EPolygonDrawMode::Fill);
    config.allowedDrawModes.emplace_back(EPolygonDrawMode::Line);

    config.primitiveTopology = Topology;

    config.renderpassProps.bOneRtPerFormat = true;
    config.renderpassProps.multisampleCount = EPixelSampleCount::SampleCount1;
    config.renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::BGRA_U8_Norm);
    config.renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::D24S8_U32_DNorm_SInt);
    config.renderpassProps.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;

    config.depthState.bEnableWrite = DepthWrite;

    AttachmentBlendState blendState;
    blendState.bBlendEnable = true;
    blendState.colorBlendOp = EBlendOp::Add;
    blendState.srcColorFactor = EBlendFactor::SrcAlpha;
    blendState.dstColorFactor = EBlendFactor::OneMinusSrcAlpha;
    blendState.alphaBlendOp = EBlendOp::Add;
    blendState.srcAlphaFactor = blendState.dstAlphaFactor = EBlendFactor::One;
    config.attachmentBlendStates.emplace_back(blendState);

    return config;
}

//////////////////////////////////////////////////////////////////////////
/// Shaders
//////////////////////////////////////////////////////////////////////////


//
// Draws Simple3D colored per vertex and uses view and instance data to transform vertices
// Shader name + Topology + (DWrite if depth writing)
//
template <EPrimitiveTopology::Type Topology, bool DepthWrite>
class Draw3DColoredPerVertex : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(Draw3DColoredPerVertex, <EXPAND_ARGS(Topology, DepthWrite)>, UniqueUtilityShaderConfig, );
private:
    String shaderFileName;

    Draw3DColoredPerVertex()
        : BaseType(String(DRAW_3D_COLORED_PER_VERTEX_NAME) + EPrimitiveTopology::getChar<Topology>() + (DepthWrite? "DWrite" : ""))
        , shaderFileName(DRAW_3D_COLORED_PER_VERTEX_NAME)
    {
        static CREATE_GRAPHICS_PIPELINE_REGISTRANT(DRAW_3D_COLORED_PER_VERTEX_REGISTER, getResourceName(), &drawSimple3DPipelineConfig<EXPAND_ARGS(Topology, DepthWrite)>);
    }

protected:
    /* UniqueUtilityShader overrides */
    String getShaderFileName() const override { return shaderFileName; }
    /* overrides ends */
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        auto ShaderParamInfoInit = [this]
        {
            std::map<String, ShaderBufferParamInfo*> paramInfo;
            paramInfo.insert(RenderSceneBase::sceneViewParamInfo().cbegin(), RenderSceneBase::sceneViewParamInfo().cend());

            // Vertex based instance param info at set 1
            auto instanceParamInfo = MaterialVertexUniforms::bufferParamInfo(vertexUsage());
            paramInfo.insert(instanceParamInfo.cbegin(), instanceParamInfo.cend());
            return paramInfo;
        };
        static std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            ShaderParamInfoInit()
        };

        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};
DEFINE_TEMPLATED_GRAPHICS_RESOURCE(Draw3DColoredPerVertex, <EXPAND_ARGS(EPrimitiveTopology::Type Topology, bool DepthWrite)>, <EXPAND_ARGS(Topology, DepthWrite)>)
template Draw3DColoredPerVertex<EPrimitiveTopology::Triangle, false>;
template Draw3DColoredPerVertex<EPrimitiveTopology::Line, false>;
template Draw3DColoredPerVertex<EPrimitiveTopology::Point, false>;

template Draw3DColoredPerVertex<EPrimitiveTopology::Triangle, true>;
template Draw3DColoredPerVertex<EPrimitiveTopology::Line, true>;

//
// Draws Simple3D colored per instance and uses view and model data from vertex per instance to transform vertices
//
class Draw3DColoredPerInstance : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(Draw3DColoredPerInstance, , UniqueUtilityShaderConfig, );
private:
    Draw3DColoredPerInstance()
        : BaseType(DRAW_3D_COLORED_PER_INSTANCE_NAME)
    {}

protected:

    /* UniqueUtilityShader overrides */
    EVertexType::Type vertexUsed() const override
    {
        return EVertexType::InstancedSimple3DColor;
    }
    /* overrides ends */

public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            RenderSceneBase::sceneViewParamInfo()
        };

        SHADER_PARAMS_INFO.insert(RenderSceneBase::sceneViewParamInfo().cbegin(), RenderSceneBase::sceneViewParamInfo().cend());

        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};
DEFINE_GRAPHICS_RESOURCE(Draw3DColoredPerInstance)

//
// Draws Simple3D colored per vertex, vertices are already transformed to world space and uses view data to transform vertices
// Shader name + Topology + (DWrite if depth writing)
//
template <EPrimitiveTopology::Type Topology, bool DepthWrite>
class DirectDraw3DColoredPerVertex : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DirectDraw3DColoredPerVertex, <EXPAND_ARGS(Topology, DepthWrite)>, UniqueUtilityShaderConfig, );
private:
    String shaderFileName;

    DirectDraw3DColoredPerVertex()
        : BaseType(String(DIRECT_DRAW_3D_COLORED_PER_VERTEX_NAME) + EPrimitiveTopology::getChar<Topology>() + (DepthWrite ? "DWrite" : ""))
        , shaderFileName(DIRECT_DRAW_3D_COLORED_PER_VERTEX_NAME)
    {
        static CREATE_GRAPHICS_PIPELINE_REGISTRANT(DRAW_3D_COLORED_PER_VERTEX_REGISTER, getResourceName(), &drawSimple3DPipelineConfig<EXPAND_ARGS(Topology, DepthWrite)>);
    }

protected:
    /* UniqueUtilityShader overrides */
    String getShaderFileName() const override { return shaderFileName; }
    /* overrides ends */
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            RenderSceneBase::sceneViewParamInfo()
        };

        SHADER_PARAMS_INFO.insert(RenderSceneBase::sceneViewParamInfo().cbegin(), RenderSceneBase::sceneViewParamInfo().cend());

        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};
DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DirectDraw3DColoredPerVertex, <EXPAND_ARGS(EPrimitiveTopology::Type Topology, bool DepthWrite)>, <EXPAND_ARGS(Topology, DepthWrite)>)
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Triangle, false>;
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Line, false>;
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Point, false>;

template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Triangle, true>;
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Line, true>;

//
// Draws Simple3D colored per instance, vertices are already transformed to world space and uses view data to transform vertices and Push constant to determine color per instance
//
class DirectDraw3DColoredPerInstance : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DirectDraw3DColoredPerInstance, , UniqueUtilityShaderConfig, );
private:
    DirectDraw3DColoredPerInstance()
        : BaseType(DIRECT_DRAW_3D_COLORED_PER_INSTANCE_NAME)
    {}

public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            RenderSceneBase::sceneViewParamInfo()
        };

        SHADER_PARAMS_INFO.insert(RenderSceneBase::sceneViewParamInfo().cbegin(), RenderSceneBase::sceneViewParamInfo().cend());

        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};
DEFINE_GRAPHICS_RESOURCE(DirectDraw3DColoredPerInstance)

// Registrar
CREATE_GRAPHICS_PIPELINE_REGISTRANT(DRAW_3D_COLORED_PER_INSTANCE_REGISTER, DRAW_3D_COLORED_PER_INSTANCE_NAME, &drawSimple3DPipelineConfig<EXPAND_ARGS(EPrimitiveTopology::Triangle, false)>);
CREATE_GRAPHICS_PIPELINE_REGISTRANT(DIRECT_DRAW_3D_COLORED_PER_INSTANCE_REGISTER, DIRECT_DRAW_3D_COLORED_PER_INSTANCE_NAME, &drawSimple3DPipelineConfig<EXPAND_ARGS(EPrimitiveTopology::Triangle, false)>);