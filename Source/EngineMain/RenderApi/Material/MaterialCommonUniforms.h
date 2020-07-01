#pragma once
#include "../../Core/Math/Matrix4.h"
#include "../VertexData.h"
#include <map>


// Base vertex instance specific data
struct InstanceData
{
    Matrix4 model;
    Matrix4 invModel;
};


namespace MaterialVertexUniforms
{
    // Vertex specific buffer info for shader descriptors
    template<EVertexType::Type VertexType>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo();

    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<Simple2>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<Simple3>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<Simple4>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<BasicMesh>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<StaticMesh>();

    constexpr const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo(EVertexType::Type vertexType);
}