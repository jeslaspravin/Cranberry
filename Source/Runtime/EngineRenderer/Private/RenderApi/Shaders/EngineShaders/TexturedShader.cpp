/*!
 * \file TexturedShader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/Shaders/EngineShaders/TexturedShader.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#define TEXTURED_SHADER_NAME TCHAR("Textured")

struct TexturedMeshMaterials
{
    TexturedMeshData *meshData;
};

BEGIN_BUFFER_DEFINITION(TexturedMeshData)
ADD_BUFFER_TYPED_FIELD(meshColor)
ADD_BUFFER_TYPED_FIELD(rm_uvScale)
ADD_BUFFER_TYPED_FIELD(diffuseMapIdx)
ADD_BUFFER_TYPED_FIELD(normalMapIdx)
ADD_BUFFER_TYPED_FIELD(armMapIdx)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(TexturedMeshMaterials)
ADD_BUFFER_STRUCT_FIELD(meshData, TexturedMeshData)
END_BUFFER_DEFINITION();

template <EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat>
class TexturedShader : public DrawMeshShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(TexturedShader, <EXPAND_ARGS(VertexUsage, RenderpassFormat)>, DrawMeshShaderConfig, )
protected:
    TexturedShader()
        : BaseType(TEXTURED_SHADER_NAME)
    {
        compatibleRenderpassFormat = RenderpassFormat;
        compatibleVertex = VertexUsage;
    }

    void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        static TexturedMeshMaterialsBufferParamInfo MESH_MATERIALS_DATA;
        static const std::map<StringID, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{
            {TCHAR("materials"), &MESH_MATERIALS_DATA}
        };

        for (const std::pair<const StringID, ShaderBufferParamInfo *> &bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(TexturedShader, <EXPAND_ARGS(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>, <EXPAND_ARGS(VertexUsage, RenderpassFormat)>)

template class TexturedShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffer>;

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

CREATE_GRAPHICS_PIPELINE_REGISTRANT(
    TEXTURED_SHADER_PIPELINE_REGISTER, TEXTURED_SHADER_NAME, &CommonGraphicsPipelineConfigs::writeGbufferShaderConfig
);
