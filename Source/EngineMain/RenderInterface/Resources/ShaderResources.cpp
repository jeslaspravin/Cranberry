#include "ShaderResources.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Logger/Logger.h"

//////////////////////////////////////////////////////////////////////////
// ShaderCodeResource
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderCodeResource)

ShaderCodeResource::ShaderCodeResource(const String& shaderName, const String& entryPointName, const uint8* shaderCodePtr)
    : BaseType()
    , shaderFileName(shaderName)
    , shaderEntryPoint(entryPointName)
    , shaderCode(shaderCodePtr)
{}

void ShaderCodeResource::init()
{
    BaseType::init();
    // Don't need since will always be subresource of shader resources
    //reinitResources();
}

String ShaderCodeResource::getResourceName() const
{
    return shaderFileName;
}

EShaderStage::Type ShaderCodeResource::shaderStage() const
{
    fatalAssert(false, "Not implemented");
    return EShaderStage::Compute/*0*/;
}

const String& ShaderCodeResource::entryPoint() const
{
    return shaderEntryPoint;
}

//////////////////////////////////////////////////////////////////////////
// ShaderResource
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ShaderResource)


ShaderResource::ShaderResource(const String& name /*= ""*/) : BaseType()
{
    shaderName = name;
}

void ShaderResource::init()
{
    BaseType::init();
    for (std::pair<const EShaderStage::Type, SharedPtr<ShaderCodeResource>>& shaderOfType : shaders)
    {
        shaderOfType.second->init();
    }
    reinitResources();
}

void ShaderResource::reinitResources()
{
    BaseType::reinitResources();
    for (std::pair<const EShaderStage::Type, SharedPtr<ShaderCodeResource>>& shaderOfType : shaders)
    {
        shaderOfType.second->reinitResources();
    }
}

void ShaderResource::release()
{
    for (std::pair<const EShaderStage::Type, SharedPtr<ShaderCodeResource>>& shaderOfType : shaders)
    {
        shaderOfType.second->release();
    }
    shaders.clear();
    BaseType::release();
}

String ShaderResource::getResourceName() const
{
    return shaderName;
}

String ShaderResource::getShaderFileName() const
{
    return getResourceName();
}

SharedPtr<ShaderCodeResource> ShaderResource::getShaderCode(EShaderStage::Type shaderType) const
{
    auto foundItr = shaders.find(shaderType);
    if (foundItr != shaders.end())
    {
        return foundItr->second;
    }
    return nullptr;
}

const std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>>& ShaderResource::getShaders() const
{
    return shaders;
}
