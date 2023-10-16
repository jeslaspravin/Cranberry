/*!
 * \file ShaderObjectFactory.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/Rendering/ShaderObjectFactory.h"
#include "RenderApi/Rendering/ShaderObject.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"

ShaderObjectBase *ShaderObjectFactory::create(const String &shaderName, const ShaderResource *shader) const
{
    if (shader->getShaderConfig()->getType()->isChildOf<DrawMeshShaderConfig>())
    {
        return new DrawMeshShaderObject(shaderName);
    }
    else if (shader->getShaderConfig()->getType()->isChildOf<UniqueUtilityShaderConfig>())
    {
        return new UniqueUtilityShaderObject(shaderName, shader);
    }
    else if (shader->getShaderConfig()->getType()->isChildOf<ComputeShaderConfig>())
    {
        return new ComputeShaderObject(shaderName, shader);
    }
    fatalAssertf(false, "Not supported shader to wrap with shader object");
    return nullptr;
}
