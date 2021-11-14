#pragma once
#include "Types/Patterns/FactoriesBase.h"
#include "EngineRendererExports.h"

class ShaderObjectBase;
class String;
class ShaderResource;

class ENGINERENDERER_EXPORT ShaderObjectFactory : public FactoriesBase<ShaderObjectBase*, const String&, const ShaderResource*>
{

public:
    ShaderObjectBase* create(const String& shaderName, const ShaderResource* shader) const override;
};
