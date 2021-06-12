#include "VulkanShaderResources.h"
#include "../../Core/Engine/GameEngine.h"
#include "../VulkanGraphicsHelper.h"
#include "../VulkanInternals/Debugging.h"
#include "../../Core/Logger/Logger.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Platform/LFS/PlatformLFS.h"
#include "ShaderArchive.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderCodeResource, VK_OBJECT_TYPE_SHADER_MODULE)

VulkanShaderCodeResource::VulkanShaderCodeResource(const String& shaderName, const ShaderStageDescription* desc, const uint8* shaderCodePtr)
    : BaseType(shaderName,desc->entryPoint, shaderCodePtr)
    , stageDescription(desc)
    , shaderModule(nullptr)
{}

VulkanShaderCodeResource::VulkanShaderCodeResource()
    : BaseType()
    , stageDescription(nullptr)
    , shaderModule(nullptr)
{}

void VulkanShaderCodeResource::reinitResources()
{
    release();
    BaseType::reinitResources();

    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    // Multiplying by sizeof(uint32) as reflection creates every calculation in uint32
    shaderModule = VulkanGraphicsHelper::createShaderModule(graphicsInstance,shaderCode + getStageDesc().codeView.startIdx * sizeof(uint32)
        , getStageDesc().codeView.size*sizeof(uint32));
    if (shaderModule != nullptr)
    {
        VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(this);
    }
}

void VulkanShaderCodeResource::release()
{
    if (shaderModule)
    {
        VulkanGraphicsHelper::destroyShaderModule(gEngine->getRenderApi()->getGraphicsInstance(), shaderModule);
        shaderModule = nullptr;
    }
    BaseType::release();
}

String VulkanShaderCodeResource::getObjectName() const
{
    return getResourceName();
}

uint64 VulkanShaderCodeResource::getDispatchableHandle() const
{
    return (uint64)shaderModule;
}

String VulkanShaderCodeResource::getResourceName() const 
{
    return BaseType::getResourceName() + EShaderStage::getShaderStageInfo(shaderStage())->shortName;
}

EShaderStage::Type VulkanShaderCodeResource::shaderStage() const
{
    return EShaderStage::Type(stageDescription->stage);
}

const ShaderStageDescription& VulkanShaderCodeResource::getStageDesc() const
{
    debugAssert(stageDescription != nullptr);
    return *stageDescription;
}

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderResource, VK_OBJECT_TYPE_SHADER_MODULE)

VulkanShaderResource::VulkanShaderResource(const String& name) : BaseType(name)
{}

String VulkanShaderResource::getObjectName() const
{
    return getResourceName();
}

void VulkanShaderResource::init()
{
    String filePath;
    filePath = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(filePath), "Shaders", getShaderFileName());
    shaderFilePath = filePath + "." + SHADER_EXTENSION;
    reflectionsFilePath = filePath + "." + REFLECTION_EXTENSION;
    PlatformFile shaderFile(shaderFilePath);
    shaderFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);
    shaderFile.addSharingFlags(EFileSharing::NoSharing);
    shaderFile.addAttributes(EFileAdditionalFlags::ReadOnly);
    PlatformFile reflectionFile(reflectionsFilePath);
    reflectionFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);
    reflectionFile.addSharingFlags(EFileSharing::NoSharing);
    reflectionFile.addAttributes(EFileAdditionalFlags::ReadOnly);

    fatalAssert(shaderFile.exists() && reflectionFile.exists(), "Shader and reflection files are mandatory in shader %s", getResourceName().getChar());
    shaderFile.openFile();
    reflectionFile.openFile();

    std::vector<uint8> reflectionData;
    shaderFile.read(shaderCode);
    shaderFile.closeFile();
    reflectionFile.read(reflectionData);
    reflectionFile.closeFile();

    // Ensure shader code is multiple of 4bytes as it is supposed to be
    debugAssert(shaderCode.size() % sizeof(uint32) == 0);
    ShaderArchive archive(reflectionData);
    archive << reflectedData;

    for (ShaderStageDescription& stageDesc : reflectedData.stages)
    {
        shaders[EShaderStage::Type(stageDesc.stage)] = SharedPtr<ShaderCodeResource>(new VulkanShaderCodeResource(getResourceName(), &stageDesc, shaderCode.data()));
    }

    BaseType::init();
}

const ShaderReflected* VulkanShaderResource::getReflection() const
{
    return &reflectedData;
}
