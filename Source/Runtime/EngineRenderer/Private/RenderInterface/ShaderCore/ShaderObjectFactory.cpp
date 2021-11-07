#include "RenderInterface/ShaderCore/ShaderObjectFactory.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/ShaderCore/ShaderObject.h"

ShaderObjectBase* ShaderObjectFactory::create(const String& shaderName, const ShaderResource* shader) const
{
    if (shader->getType()->isChildOf<DrawMeshShader>())
    {
        return new DrawMeshShaderObject(shaderName);
    }
    else if (shader->getType()->isChildOf<UniqueUtilityShader>())
    {
        return new UniqueUtilityShaderObject(shaderName, shader);
    }
    else if (shader->getType()->isChildOf<ComputeShader>())
    {
        return new ComputeShaderObject(shaderName, shader);
    }
    fatalAssert(false, "Not supported shader to wrap with shader object");
    return nullptr;
}
