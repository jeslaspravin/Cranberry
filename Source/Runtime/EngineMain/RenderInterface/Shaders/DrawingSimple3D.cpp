#include "../../RenderInterface/ShaderCore/ShaderParameters.h"
#include "../../RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "../../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../RenderApi/Scene/RenderScene.h"
#include "../../RenderApi/Material/MaterialCommonUniforms.h"

#define DRAW_3D_COLORED_PER_VERTEX_NAME "Draw3DColoredPerVertex"
#define DRAW_3D_COLORED_PER_INSTANCE_NAME "Draw3DColoredPerInstance"
#define DIRECT_DRAW_3D_COLORED_PER_VERTEX_NAME "DirectDraw3DColoredPerVertex"
#define DIRECT_DRAW_3D_COLORED_PER_INSTANCE_NAME "DirectDraw3DColoredPerInstance"


//////////////////////////////////////////////////////////////////////////
/// Pipeline start
//////////////////////////////////////////////////////////////////////////

template <EPrimitiveTopology::Type Topology, bool DepthWrite>
class DrawSimple3DShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(DrawSimple3DShaderPipeline, <ExpandArgs(Topology, DepthWrite)>, GraphicsPipeline, )
protected:
    DrawSimple3DShaderPipeline() = default;
public:
    DrawSimple3DShaderPipeline(const PipelineBase* parent);
    DrawSimple3DShaderPipeline(const ShaderResource* shaderResource);
};
DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DrawSimple3DShaderPipeline, <ExpandArgs(EPrimitiveTopology::Type Topology, bool DepthWrite)>, <ExpandArgs(Topology, DepthWrite)>)

template <EPrimitiveTopology::Type Topology, bool DepthWrite>
DrawSimple3DShaderPipeline<Topology, DepthWrite>::DrawSimple3DShaderPipeline(const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

template <EPrimitiveTopology::Type Topology, bool DepthWrite>
DrawSimple3DShaderPipeline<Topology, DepthWrite>::DrawSimple3DShaderPipeline(const ShaderResource* shaderResource)
    : BaseType()
{
    setPipelineShader(shaderResource);
    setResourceName("DrawSimple3D_" + shaderResource->getResourceName());
    supportedCullings.emplace_back(ECullingMode::BackFace);

    allowedDrawModes.emplace_back(EPolygonDrawMode::Fill);
    allowedDrawModes.emplace_back(EPolygonDrawMode::Line);

    primitiveTopology = Topology;

    renderpassProps.bOneRtPerFormat = true;
    renderpassProps.multisampleCount = EPixelSampleCount::SampleCount1;
    renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::BGRA_U8_Norm);
    renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::D24S8_U32_DNorm_SInt);
    renderpassProps.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;

    depthState.bEnableWrite = DepthWrite;

    AttachmentBlendState blendState;
    blendState.bBlendEnable = true;
    blendState.colorBlendOp = EBlendOp::Add;
    blendState.srcColorFactor = EBlendFactor::SrcAlpha;
    blendState.dstColorFactor = EBlendFactor::OneMinusSrcAlpha;
    blendState.alphaBlendOp = EBlendOp::Add;
    blendState.srcAlphaFactor = blendState.dstAlphaFactor = EBlendFactor::One;
    attachmentBlendStates.emplace_back(blendState);
}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////
template <EPrimitiveTopology::Type Topology, bool DepthWrite>
using DrawSimple3DShaderPipelineRegistrar = GenericPipelineRegistrar<DrawSimple3DShaderPipeline<Topology, DepthWrite>>;

//////////////////////////////////////////////////////////////////////////
/// Shaders
//////////////////////////////////////////////////////////////////////////


//
// Draws Simple3D colored per vertex and uses view and instance data to transform vertices
// Shader name + Topology + (DWrite if depth writing)
//
template <EPrimitiveTopology::Type Topology, bool DepthWrite>
class Draw3DColoredPerVertex : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(Draw3DColoredPerVertex, <ExpandArgs(Topology, DepthWrite)>, UniqueUtilityShader, );
private:
    String shaderFileName;

    Draw3DColoredPerVertex()
        : BaseType(String(DRAW_3D_COLORED_PER_VERTEX_NAME) + EPrimitiveTopology::getChar<Topology>() + (DepthWrite? "DWrite" : ""))
        , shaderFileName(DRAW_3D_COLORED_PER_VERTEX_NAME)
    {
        static DrawSimple3DShaderPipelineRegistrar<Topology, DepthWrite> DRAW_3D_COLORED_PER_VERTEX_REGISTER(getResourceName());
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
DEFINE_TEMPLATED_GRAPHICS_RESOURCE(Draw3DColoredPerVertex, <ExpandArgs(EPrimitiveTopology::Type Topology, bool DepthWrite)>, <ExpandArgs(Topology, DepthWrite)>)
template Draw3DColoredPerVertex<EPrimitiveTopology::Triangle, false>;
template Draw3DColoredPerVertex<EPrimitiveTopology::Line, false>;
template Draw3DColoredPerVertex<EPrimitiveTopology::Point, false>;

template Draw3DColoredPerVertex<EPrimitiveTopology::Triangle, true>;
template Draw3DColoredPerVertex<EPrimitiveTopology::Line, true>;

//
// Draws Simple3D colored per instance and uses view and model data from vertex per instance to transform vertices
//
class Draw3DColoredPerInstance : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(Draw3DColoredPerInstance, , UniqueUtilityShader, );
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
class DirectDraw3DColoredPerVertex : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(DirectDraw3DColoredPerVertex, <ExpandArgs(Topology, DepthWrite)>, UniqueUtilityShader, );
private:
    String shaderFileName;

    DirectDraw3DColoredPerVertex()
        : BaseType(String(DIRECT_DRAW_3D_COLORED_PER_VERTEX_NAME) + EPrimitiveTopology::getChar<Topology>() + (DepthWrite ? "DWrite" : ""))
        , shaderFileName(DIRECT_DRAW_3D_COLORED_PER_VERTEX_NAME)
    {
        static DrawSimple3DShaderPipelineRegistrar<Topology, DepthWrite> DRAW_3D_COLORED_PER_VERTEX_REGISTER(getResourceName());
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
DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DirectDraw3DColoredPerVertex, <ExpandArgs(EPrimitiveTopology::Type Topology, bool DepthWrite)>, <ExpandArgs(Topology, DepthWrite)>)
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Triangle, false>;
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Line, false>;
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Point, false>;

template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Triangle, true>;
template DirectDraw3DColoredPerVertex<EPrimitiveTopology::Line, true>;

//
// Draws Simple3D colored per instance, vertices are already transformed to world space and uses view data to transform vertices and Push constant to determine color per instance
//
class DirectDraw3DColoredPerInstance : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(DirectDraw3DColoredPerInstance, , UniqueUtilityShader, );
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
DrawSimple3DShaderPipelineRegistrar<EPrimitiveTopology::Triangle, false> DRAW_3D_COLORED_PER_INSTANCE_REGISTER(DRAW_3D_COLORED_PER_INSTANCE_NAME);
DrawSimple3DShaderPipelineRegistrar<EPrimitiveTopology::Triangle, false> DIRECT_DRAW_3D_COLORED_PER_INSTANCE_REGISTER(DIRECT_DRAW_3D_COLORED_PER_INSTANCE_NAME);