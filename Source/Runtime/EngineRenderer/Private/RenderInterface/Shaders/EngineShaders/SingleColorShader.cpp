/*!
 * \file SingleColorShader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Shaders/EngineShaders/SingleColorShader.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#define SINGLECOLOR_SHADER_NAME TCHAR("SingleColor")

struct SingleColorMeshMaterials
{
    SingleColorMeshData *meshData;
};

BEGIN_BUFFER_DEFINITION(SingleColorMeshData)
ADD_BUFFER_TYPED_FIELD(meshColor)
ADD_BUFFER_TYPED_FIELD(roughness)
ADD_BUFFER_TYPED_FIELD(metallic)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(SingleColorMeshMaterials)
ADD_BUFFER_STRUCT_FIELD(meshData, SingleColorMeshData)
END_BUFFER_DEFINITION();

template <EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat>
class SingleColorShader : public DrawMeshShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(
        SingleColorShader, <EXPAND_ARGS(VertexUsage, RenderpassFormat)>, DrawMeshShaderConfig, )
protected:
    SingleColorShader()
        : BaseType(SINGLECOLOR_SHADER_NAME)
    {
        compatibleRenderpassFormat = RenderpassFormat;
        compatibleVertex = VertexUsage;
    }

    void bindBufferParamInfo(
        std::map<String, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        static SingleColorMeshMaterialsBufferParamInfo MESH_DATA_MATERIALS;
        static const std::map<String, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{ { TCHAR("materials"),
            &MESH_DATA_MATERIALS } };

        for (const std::pair<const String, ShaderBufferParamInfo *> &bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(SingleColorShader,
    <EXPAND_ARGS(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>,
    <EXPAND_ARGS(VertexUsage, RenderpassFormat)>)

template class SingleColorShader<EVertexType::Simple2, ERenderPassFormat::Multibuffer>;
template class SingleColorShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffer>;

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

CREATE_GRAPHICS_PIPELINE_REGISTRANT(SINGLECOLOR_SHADER_PIPELINE_REGISTER, SINGLECOLOR_SHADER_NAME,
    &CommonGraphicsPipelineConfigs::writeGbufferShaderConfig);
