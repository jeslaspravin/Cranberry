/*!
 * \file ShaderObjectFactory.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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
