#pragma once
#include "GraphicsResources.h"
#include "../../Core/String/String.h"
#include "../../Core/Platform/LFS/PlatformLFS.h"
#include "../../Core/Types/Functions.h"

#include <map>

namespace EShaderStage
{
    enum Type
    {
        Compute,
        Vertex,
        TessellationControl,
        TessellatonEvaluate,
        Geometry,
        Fragment
    };
    
    struct ShaderStageInfo
    {
        String name;
        String shortName;
        String entryPointName;
        uint32 shaderStage;
    };

    const ShaderStageInfo* getShaderStageInfo(EShaderStage::Type shaderStage);
}

// Shader subresource for ShaderResource
class ShaderCodeResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderCodeResource,,GraphicsResource,)

private:
    String fileName;
    PlatformFile shaderFile;
protected:
    std::vector<uint8> shaderCode;

    ShaderCodeResource() = default;
public:
    ShaderCodeResource(const String& filePath);

    void init() override;
    virtual void reinitResources() override;
    virtual void release() override;
    String getResourceName() const override;
    void setResourceName(const String& name) override {}
};

class ShaderResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderResource,,GraphicsResource,)

private:
    String shaderName;
protected:
    using ShaderCodeFactory = ClassFunction<ShaderResource,ShaderCodeResource*, const String&>;
    ShaderCodeFactory shaderCodeFactory;
    std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>> shaders;

    ShaderResource(const String& name = "");
public:
    const String SHADER_EXTENSION = "shader";

    /* GraphicsResource overrides */
    void init() override;
    void reinitResources() override;
    void release() override;

    String getResourceName() const override;
    void setResourceName(const String& name) override {}

    /* End overrides */

    SharedPtr<ShaderCodeResource> getShaderCode(EShaderStage::Type shaderType) const;
    const std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>>& getShaders() const;
};