#pragma once
#include "../../Core/Types/Patterns/FactoriesBase.h"

class ShaderObjectBase;
class String;
class ShaderResource;

class ShaderObjectFactory : public FactoriesBase<ShaderObjectBase, const String&, const ShaderResource*>
{

public:
	ShaderObjectBase* create(const String& shaderName, const ShaderResource* shader) const override;
};
