/*!
 * \file VertexData.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "EngineRendererExports.h"
#include "Math/Vector4.h"
#include "Math/Vector2.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderInterface/Resources/ShaderResources.h"

struct SpecializationConstantEntry;

struct StaticMeshVertex
{
    Vector4 position; // xyz position, w texture coord's U
    Vector4 normal;   // xyz normal, w texture coord's V
    Vector4 tangent;
};

// IMGui compatible vertex, Needs one draw call per texture per layer
struct VertexUI
{
    Vector2 position;
    Vector2 uv;
    uint32 color;
};

namespace EVertexType
{
// Also update in "MaterialCommonUniforms.h" MaterialVertexUniforms
enum Type
{
    Simple2,       // position only vertices(vec2)
    UI,            // Currently ImGui vertices
    Simple3,       // Position only vertices(vec3)
    Simple3DColor, // position vertices & color(vec4)
    BasicMesh,     // Basic mesh with position, texture coordinates
    StaticMesh,
    InstancedSimple3DColor,
    NoVertex,
    MaxVertexType,
    TypeStart = Simple2,
    TypeEnd = NoVertex
};

ENGINERENDERER_EXPORT String toString(Type vertexType);

// Each index corresponds to binding of a vertex struct
template <Type VertexType>
const std::vector<ShaderVertexParamInfo *> &vertexParamInfo();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<Simple2>();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<UI>();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<Simple3>();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<Simple3DColor>();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<BasicMesh>();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<StaticMesh>();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<InstancedSimple3DColor>();
template <>
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo<NoVertex>();

/**
 * Vertex layout for Input assembler shader stage, This will be filled when GlobalRenderingContext initializes.
 * Consumed by Graphics Pipeline's input assembler
 */
ENGINERENDERER_EXPORT const std::vector<ShaderVertexParamInfo *> &vertexParamInfo(Type vertexType);

template <Type VertexType>
void vertexSpecConsts(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<Simple2>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<UI>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<Simple3>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<Simple3DColor>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<BasicMesh>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<StaticMesh>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<StaticMesh>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<InstancedSimple3DColor>(SpecConstantNamedMap &specializationConst);
template <>
ENGINERENDERER_EXPORT void vertexSpecConsts<NoVertex>(SpecConstantNamedMap &specializationConst);

/**
 * Vertex Specialization constant is consumed by DrawMeshShaderConfig.
 */
ENGINERENDERER_EXPORT void vertexSpecConsts(Type vertexType, SpecConstantNamedMap &specializationConst);
} // namespace EVertexType