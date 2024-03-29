/*!
 * \file MaterialCommonUniforms.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <map>

#include "EngineRendererExports.h"
#include "Math/Matrix4.h"
#include "RenderApi/VertexData.h"

// Base vertex instance specific data
struct InstanceData
{
    Matrix4 model;
    Matrix4 invModel;
    // Index to shader unique param index
    uint32 shaderUniqIdx;
};

namespace MaterialVertexUniforms
{
// Vertex specific buffer info for shader descriptors
template <EVertexType::Type VertexType>
const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo();

template <>
ENGINERENDERER_EXPORT const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo<EVertexType::Simple2>();
template <>
ENGINERENDERER_EXPORT const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo<EVertexType::UI>();
template <>
ENGINERENDERER_EXPORT const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo<EVertexType::Simple3>();
template <>
ENGINERENDERER_EXPORT const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo<EVertexType::Simple3DColor>();
template <>
ENGINERENDERER_EXPORT const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo<EVertexType::BasicMesh>();
template <>
ENGINERENDERER_EXPORT const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo<EVertexType::StaticMesh>();
template <>
ENGINERENDERER_EXPORT const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo<EVertexType::NoVertex>();

template <EVertexType::Type VertexType>
const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo()
{
    static const std::map<StringID, ShaderBufferParamInfo *> NoInstanceDataForThisVertex;
    return NoInstanceDataForThisVertex;
}

/**
 * Vertex Unique shader descriptor set will use this param info to fill uniform's structure
 * This will be filled when VulkanVertexUniqDescLayout Descriptor set layout is created
 * Consumed whenever ShaderParameter is created with corresponding layout
 */
ENGINERENDERER_EXPORT constexpr const std::map<StringID, ShaderBufferParamInfo *> &bufferParamInfo(EVertexType::Type vertexType)
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
        return bufferParamInfo<EVertexType::InstancedSimple3DColor>();
    case EVertexType::NoVertex:
    default:
        return bufferParamInfo<EVertexType::NoVertex>();
    }
}
} // namespace MaterialVertexUniforms