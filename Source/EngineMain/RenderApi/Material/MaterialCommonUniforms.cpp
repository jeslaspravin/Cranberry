#include "MaterialCommonUniforms.h"

BEGIN_BUFFER_DEFINITION(InstanceData)
ADD_BUFFER_TYPED_FIELD(model)
ADD_BUFFER_TYPED_FIELD(invModel)
END_BUFFER_DEFINITION();


namespace MaterialVertexUniforms
{
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<Simple2>()
    {
        static InstanceDataBufferParamInfo instanceDataBufferInfo;
        static const std::map<String, ShaderBufferParamInfo*> VERTEX_BUFFER_PARAMS
        {
            { "instanceData", &instanceDataBufferInfo }
        };

        return VERTEX_BUFFER_PARAMS;
    }

    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<Simple3>()
    {
        return bufferParamInfo<Simple2>();
    }
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<Simple4>()
    {
        return bufferParamInfo<Simple2>();
    }
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<BasicMesh>()
    {
        return bufferParamInfo<Simple2>();
    }
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<StaticMesh>()
    {
        return bufferParamInfo<Simple2>();
    }

    constexpr const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo(EVertexType::Type vertexType)
    {
        switch (vertexType)
        {
        case EVertexType::Simple2:
            return bufferParamInfo<Simple2>();
        case EVertexType::Simple3:
            return bufferParamInfo<Simple3>();
        case EVertexType::Simple4:
            return bufferParamInfo<Simple4>();
        default:
        case EVertexType::BasicMesh:
            return bufferParamInfo<BasicMesh>();
        case EVertexType::StaticMesh:
            return bufferParamInfo<StaticMesh>();
        }
    }
}