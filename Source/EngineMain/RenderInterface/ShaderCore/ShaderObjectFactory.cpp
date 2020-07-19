#include "ShaderObjectFactory.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../Shaders/Base/DrawMeshShader.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "ShaderObject.h"

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
    fatalAssert(false, "Not supported shader to wrap with shader object");
    return nullptr;
}
