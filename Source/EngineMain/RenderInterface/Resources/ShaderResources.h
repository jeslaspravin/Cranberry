#pragma once
#include "GraphicsResources.h"
#include "../../Core/String/String.h"
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
        // TODO(Jeslas) : Entry point is obtained from reflection data right now, so do I need here?
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
    String shaderFileName;
    String shaderEntryPoint;
protected:
    const uint8* shaderCode;

    ShaderCodeResource() = default;
public:
    ShaderCodeResource(const String& shaderName,const String& entryPointName, const uint8* shaderCodePtr);

    /* GraphicsResources overrides */
    void init() override;
    String getResourceName() const override;
    void setResourceName(const String& name) override {}
    /* End overrides */

    const String& entryPoint() const;
    virtual EShaderStage::Type shaderStage() const;
};

class ShaderResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderResource,,GraphicsResource,)

private:
    String shaderName;
protected:
    const String SHADER_EXTENSION = "shader";
    const String REFLECTION_EXTENSION = "ref";
protected:
    std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>> shaders;

    ShaderResource(const String& name = "");
public:

    /* GraphicsResource overrides */
    void init() override;
    void reinitResources() override;
    void release() override;

    String getResourceName() const override;
    void setResourceName(const String& name) override {}

    /* End overrides */

    virtual struct ShaderReflected const* getReflection() const { return nullptr; }
    SharedPtr<ShaderCodeResource> getShaderCode(EShaderStage::Type shaderType) const;
    const std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>>& getShaders() const;
};