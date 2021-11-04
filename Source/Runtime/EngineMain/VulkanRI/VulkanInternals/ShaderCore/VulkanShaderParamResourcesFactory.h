#pragma once
#include "../../../Core/Types/Patterns/FactoriesBase.h"
#include "../../../Core/Platform/PlatformTypes.h"

class ShaderResource;
class GraphicsResource;

class VulkanShaderParametersLayoutFactory final : public FactoriesBase<GraphicsResource, const ShaderResource*, uint32>
{

public:
    GraphicsResource* create(const ShaderResource* forShader, uint32 descriptorsSetIdx) const final;

};