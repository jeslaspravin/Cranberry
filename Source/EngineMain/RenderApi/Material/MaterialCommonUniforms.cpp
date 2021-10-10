#include "MaterialCommonUniforms.h"

BEGIN_BUFFER_DEFINITION(InstanceData)
ADD_BUFFER_TYPED_FIELD(model)
ADD_BUFFER_TYPED_FIELD(invModel)
ADD_BUFFER_TYPED_FIELD(shaderUniqIdx)
END_BUFFER_DEFINITION();

template <typename WrappingType>
struct InstancesWrapper
{
    WrappingType* instances;
};
using InstanceDataWrapper = InstancesWrapper<InstanceData>;

BEGIN_BUFFER_DEFINITION(InstanceDataWrapper)
ADD_BUFFER_STRUCT_FIELD(instances, InstanceData)
END_BUFFER_DEFINITION();

namespace MaterialVertexUniforms
{
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple2>()
    {
        static InstanceDataWrapperBufferParamInfo INSTDATA_WRAPPER_INFO;
        static const std::map<String, ShaderBufferParamInfo*> VERTEX_BUFFER_PARAMS
        {
            { "instancesWrapper", &INSTDATA_WRAPPER_INFO }
        };

        return VERTEX_BUFFER_PARAMS;
    }

    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::UI>()
    {
        return bufferParamInfo<EVertexType::Simple2>();
    }

    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple3>()
    {
        return bufferParamInfo<EVertexType::Simple2>();
    }
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple3DColor>()
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
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::NoVertex>()
    {
        return bufferParamInfo<EVertexType::Simple2>();
    }
}