#pragma once
#include "../RenderInterface/ShaderCore/ShaderParameters.h"
#include "../Core/Math/Vector4D.h"

#include <map>

struct SpecializationConstantEntry;

namespace EVertexType
{
    enum Type
    {
        Simple2,// position only vertices(vec2)
        UI,// Currently ImGui vertices
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
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<UI>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple3>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple4>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<BasicMesh>();
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<StaticMesh>();

    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo(Type vertexType);

    template<Type VertexType>
    void vertexSpecConsts(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    void vertexSpecConsts<Simple2>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    void vertexSpecConsts<UI>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    void vertexSpecConsts<Simple3>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    void vertexSpecConsts<Simple4>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    void vertexSpecConsts<BasicMesh>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    void vertexSpecConsts<StaticMesh>(std::map<String, SpecializationConstantEntry>& specializationConst);

    void vertexSpecConsts(Type vertexType, std::map<String, SpecializationConstantEntry>& specializationConst);
}