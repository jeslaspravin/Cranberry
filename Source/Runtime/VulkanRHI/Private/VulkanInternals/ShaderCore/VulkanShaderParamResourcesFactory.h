/*!
 * \file VulkanShaderParamResourcesFactory.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/CoreTypes.h"
#include "Types/Patterns/FactoriesBase.h"

class ShaderResource;
class GraphicsResource;

class VulkanShaderParametersLayoutFactory final
    : public FactoriesBase<GraphicsResource *, const ShaderResource *, uint32>
{

public:
    GraphicsResource *create(const ShaderResource *forShader, uint32 descriptorsSetIdx) const final;
};