/*!
 * \file UtilityShaders.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderApi/VertexData.h"
#include "RenderInterface/Resources/ShaderResources.h"

// Utility graphics shader or compute shaders
class ENGINERENDERER_EXPORT UniqueUtilityShaderConfig : public ShaderConfigCollector
{
    DECLARE_GRAPHICS_RESOURCE(UniqueUtilityShaderConfig, , ShaderConfigCollector, )

protected:
    UniqueUtilityShaderConfig() = default;
    UniqueUtilityShaderConfig(const String &name)
        : BaseType(name)
    {}

    virtual EVertexType::Type vertexUsed() const { return EVertexType::MaxVertexType; }

public:
    EVertexType::Type vertexUsage() const;
};

class ENGINERENDERER_EXPORT ComputeShaderConfig : public ShaderConfigCollector
{
    DECLARE_GRAPHICS_RESOURCE(ComputeShaderConfig, , ShaderConfigCollector, )
private:
    const Size3D subgrpSize;

protected:
    ComputeShaderConfig() = default;
    ComputeShaderConfig(const Size3D &subgroupSize, const String &name)
        : BaseType(name)
        , subgrpSize(subgroupSize)
    {}

public:
    const Size3D &getSubGroupSize() const { return subgrpSize; }
};

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
class ComputeShaderConfigTemplated : public ComputeShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(ComputeShaderConfigTemplated, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>, ComputeShaderConfig, )
private:
    String shaderFileName;

private:
    ComputeShaderConfigTemplated() = default;

protected:
    ComputeShaderConfigTemplated(const String &name)
        : BaseType(
            Size3D(SizeX, SizeY, SizeZ),
            name + TCHAR("_") + String::toString(SizeX) + TCHAR("x") + String::toString(SizeY) + TCHAR("x") + String::toString(SizeZ)
        )
        , shaderFileName(name)
    {}

public:
    /* ShaderResource overrides */
    String getShaderFileName() const override;

    /* End overrides */
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(ComputeShaderConfigTemplated, <EXPAND_ARGS(uint32 SizeX, uint32 SizeY, uint32 SizeZ)>, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>)

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
String ComputeShaderConfigTemplated<SizeX, SizeY, SizeZ>::getShaderFileName() const
{
    return shaderFileName;
}
