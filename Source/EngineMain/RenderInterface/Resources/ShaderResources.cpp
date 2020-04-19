#include "ShaderResources.h"
#include "../../Core/Logger/Logger.h"

DEFINE_GRAPHICS_RESOURCE(ShaderCodeResource)

ShaderCodeResource::ShaderCodeResource(const String& filePath)
    : BaseType()
    , shaderFile(filePath)
{
    fileName = shaderFile.getFileName();
    shaderFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);
    shaderFile.addSharingFlags(EFileSharing::NoSharing);
    shaderFile.addAttributes(EFileAdditionalFlags::ReadOnly);
}

void ShaderCodeResource::init()
{
    BaseType::init();
    if (!shaderFile.openFile())
    {
        Logger::error("ShaderCodeResource", "%s() : Failed opening shader file %s", __func__, fileName.getChar());
        return;
    }
    // Don't need since will always be subresource of shader resources
    //reinitResources();
}

void ShaderCodeResource::reinitResources()
{
    BaseType::reinitResources();
    shaderFile.read(shaderCode);
}

void ShaderCodeResource::release()
{
    shaderFile.closeFile();
    BaseType::release();
}

String ShaderCodeResource::getResourceName() const
{
    return fileName;
}

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
