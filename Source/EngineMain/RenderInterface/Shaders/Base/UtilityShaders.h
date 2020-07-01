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
public:
    EVertexType::Type vertexUsage() const;
};