#include "MaterialCommonUniforms.h"

BEGIN_BUFFER_DEFINITION(InstanceData)
ADD_BUFFER_TYPED_FIELD(model)
ADD_BUFFER_TYPED_FIELD(invModel)
END_BUFFER_DEFINITION();


namespace MaterialVertexUniforms
{
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple2>()
    {
        static InstanceDataBufferParamInfo instanceDataBufferInfo;
        static const std::map<String, ShaderBufferParamInfo*> VERTEX_BUFFER_PARAMS
        {
            { "instanceData", &instanceDataBufferInfo }
        };

        return VERTEX_BUFFER_PARAMS;
    }

    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple3>()
    {
        return bufferParamInfo<EVertexType::Simple2>();
    }
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple4>()
    {
        return bufferParamInfo<EVertexType::Simple2>();
    }
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::BasicMesh>()
    {
        return bufferParamInfo<EVertexType::Simple2>();
    }
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::StaticMesh>()
    {
        return bufferParamInfo<EVertexType::Simple2>();
    }
}