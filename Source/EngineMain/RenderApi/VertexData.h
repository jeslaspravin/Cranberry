#pragma once
#include "../RenderInterface/ShaderCore/ShaderParameters.h"

namespace EVertexType
{
    enum Type
    {
        Simple2,// position only vertices(vec2)
        Simple3,// Position only vertices(vec3)
        Simple4,// position only vertices(vec4)
        BasicMesh, // Basic mesh with position, texture coordinates
        StaticMesh
    };

    String toString(Type vertexType);

    // Each index corresponds to binding of a vertex struct
    template<Type VertexType>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple2>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple3>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple4>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<BasicMesh>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<StaticMesh>();

    constexpr const std::vector<ShaderVertexParamInfo*>& vertexParamInfo(Type vertexType);
}