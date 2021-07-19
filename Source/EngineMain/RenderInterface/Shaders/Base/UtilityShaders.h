#pragma once
#include "../../PlatformIndependentHeaders.h"
#include "../../../RenderApi/VertexData.h"

// Utility graphics shader or compute shaders
class UniqueUtilityShader : public GraphicsShaderResource
{
    DECLARE_GRAPHICS_RESOURCE(UniqueUtilityShader,, GraphicsShaderResource,)

private:
    UniqueUtilityShader() = default;
protected:
    UniqueUtilityShader(const String& name) : BaseType(name) {}

    virtual EVertexType::Type vertexUsed() const { return EVertexType::MaxVertexType; }
public:
    EVertexType::Type vertexUsage() const;
};

class ComputeShader : public GraphicsShaderResource
{
    DECLARE_GRAPHICS_RESOURCE(ComputeShader,, GraphicsShaderResource,)
private:
    const Size3D subgrpSize;
protected:
    ComputeShader() = default;
    ComputeShader(const Size3D& subgroupSize, const String& name)
        : BaseType(name) 
        , subgrpSize(subgroupSize)
    {}
public:

    const Size3D& getSubGroupSize() const { return subgrpSize; }
};

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
class ComputeShaderTemplated : public ComputeShader
{
    DECLARE_GRAPHICS_RESOURCE(ComputeShaderTemplated, <ExpandArgs(SizeX, SizeY, SizeZ)>, ComputeShader, )
private:
    String shaderFileName;
private:
    ComputeShaderTemplated() = default;
protected:
    ComputeShaderTemplated(const String& name)
        : BaseType(
            Size3D(SizeX, SizeY, SizeZ)
            , name + "_" + std::to_string(SizeX) + "x"+ std::to_string(SizeY) + "x" + std::to_string(SizeZ)
            )
        , shaderFileName(name)
    {}
public:
    /* ShaderResource overrides */
    String getShaderFileName() const override;

    /* End overrides */
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(ComputeShaderTemplated, <ExpandArgs(uint32 SizeX, uint32 SizeY, uint32 SizeZ)>
    , <ExpandArgs(SizeX, SizeY, SizeZ)>)

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
String ComputeShaderTemplated<SizeX, SizeY, SizeZ>::getShaderFileName() const
{
    return shaderFileName;
}
