#pragma once
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "Math/Vector4D.h"
#include "EngineRendererExports.h"

#include <map>

struct SpecializationConstantEntry;

struct StaticMeshVertex
{
    Vector4D position;// xyz position, w texture coord's U
    Vector4D normal;// xyz normal, w texture coord's V
    Vector4D tangent;
};


namespace EVertexType
{
    // Also update in "MaterialCommonUniforms.h" MaterialVertexUniforms
    enum Type
    {
        Simple2,// position only vertices(vec2)
        UI,// Currently ImGui vertices
        Simple3,// Position only vertices(vec3)
        Simple3DColor,// position vertices & color(vec4)
        BasicMesh, // Basic mesh with position, texture coordinates
        StaticMesh,
        InstancedSimple3DColor,
        NoVertex,
        MaxVertexType,
    };

    ENGINERENDERER_EXPORT String toString(Type vertexType);

    // Each index corresponds to binding of a vertex struct
    template<Type VertexType>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple2>();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<UI>();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple3>();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple3DColor>();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<BasicMesh>();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<StaticMesh>();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<InstancedSimple3DColor>();
    template<>
    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<NoVertex>();

    ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo*>& vertexParamInfo(Type vertexType);

    template<Type VertexType>
    void vertexSpecConsts(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<Simple2>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<UI>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<Simple3>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<Simple3DColor>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<BasicMesh>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<StaticMesh>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<StaticMesh>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<InstancedSimple3DColor>(std::map<String, SpecializationConstantEntry>& specializationConst);
    template<>
    ENGINERENDERER_EXPORT void vertexSpecConsts<NoVertex>(std::map<String, SpecializationConstantEntry>& specializationConst);

    ENGINERENDERER_EXPORT void vertexSpecConsts(Type vertexType, std::map<String, SpecializationConstantEntry>& specializationConst);
}