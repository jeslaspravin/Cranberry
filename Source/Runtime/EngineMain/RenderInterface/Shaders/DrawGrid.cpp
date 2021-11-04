#include "../ShaderCore/ShaderParameters.h"
#include "../ShaderCore/ShaderParameterResources.h"
#include "Base/UtilityShaders.h"
#include "Base/ScreenspaceQuadGraphicsPipeline.h"
#include "../../RenderApi/Scene/RenderScene.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

#define DRAW_GRID_NAME "DrawGrid"

template <bool DepthTest>
using DrawGridQuadPipelineBase = std::conditional_t<DepthTest, OverBlendedSSQuadWithDepthTestPipeline, OverBlendedSSQuadShaderPipeline>;

template <typename DrawGridPipelineBaseType>
class DrawGridQuadPipeline : public DrawGridPipelineBaseType
{
    DECLARE_GRAPHICS_RESOURCE(DrawGridQuadPipeline, <ExpandArgs(DrawGridPipelineBaseType)>, DrawGridPipelineBaseType, );

private:
    using DrawGridPipelineBaseType::supportedCullings;

    DrawGridQuadPipeline() = default;
public:
    DrawGridQuadPipeline(const PipelineBase * parent)
        : BaseType(parent)
    {}
    DrawGridQuadPipeline(const ShaderResource* shaderResource)
        : BaseType(shaderResource)
    {
        // Since grid has to be visible either side
        supportedCullings.clear();
        supportedCullings.emplace_back(ECullingMode::None);
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DrawGridQuadPipeline, <ExpandArgs(typename DrawGridPipelineBaseType)>, <ExpandArgs(DrawGridPipelineBaseType)>);

template <bool DepthTest>
using DrawGridQuadPipelineRegistrar = GenericPipelineRegistrar<DrawGridQuadPipeline<DrawGridQuadPipelineBase<DepthTest>>>;

template <bool DepthTest>
class DrawGrid : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(DrawGrid, <ExpandArgs(DepthTest)>, UniqueUtilityShader, );
private:
    String shaderFileName;

    DrawGrid()
        : BaseType(String(DRAW_GRID_NAME) + (DepthTest ? "DTest" : ""))
        , shaderFileName(DRAW_GRID_NAME)
    {
        static DrawGridQuadPipelineRegistrar<DepthTest> DRAW_GRID_PIPELINE_REGISTRAR(getResourceName());
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

        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DrawGrid, <ExpandArgs(bool DepthTest)>, <ExpandArgs(DepthTest)>);

template DrawGrid<false>;
template DrawGrid<true>;

