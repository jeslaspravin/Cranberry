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
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple2>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::UI>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple3>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::Simple3DColor>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::BasicMesh>();
    template<>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo<EVertexType::StaticMesh>();

    template<EVertexType::Type VertexType>
    const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo()
    {
        static const std::map<String, ShaderBufferParamInfo*> NoInstanceDataForThisVertex;
        return NoInstanceDataForThisVertex;
    }

    constexpr const std::map<String, ShaderBufferParamInfo*>& bufferParamInfo(EVertexType::Type vertexType)
    {
        switch (vertexType)
        {
        case EVertexType::Simple2:
            return bufferParamInfo<EVertexType::Simple2>();
        case EVertexType::UI:
            return bufferParamInfo<EVertexType::UI>();
        case EVertexType::Simple3:
            return bufferParamInfo<EVertexType::Simple3>();
        case EVertexType::Simple3DColor:
            return bufferParamInfo<EVertexType::Simple3DColor>();
        case EVertexType::BasicMesh:
            return bufferParamInfo<EVertexType::BasicMesh>();
        case EVertexType::StaticMesh:
            return bufferParamInfo<EVertexType::StaticMesh>();
        case EVertexType::InstancedSimple3DColor:
        default:
            return bufferParamInfo<EVertexType::InstancedSimple3DColor>();
        }
    }
}