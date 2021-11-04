#pragma once
#include "GraphicsResources.h"
#include "../../Core/String/String.h"
#include "../../Core/Types/Functions.h"
#include "../ShaderCore/ShaderInputOutput.h"

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
        const String name;
        const String shortName;
        // TODO(Jeslas) : Entry point is obtained from reflection data right now, so do I need here?
        const String entryPointName;
        const uint32 shaderStage;
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

    // Provides file name from which the shaders have to be loaded
    virtual String getShaderFileName() const;
public:

    /* GraphicsResource overrides */
    void init() override;
    void reinitResources() override;
    void release() override;

    String getResourceName() const final;
    void setResourceName(const String& name) final {}

    /* End overrides */

    virtual struct ShaderReflected const* getReflection() const { return nullptr; }
    /*
    * Binds BufferParamInfo for each buffer descriptor(Depending upon override corresponding set varies)
    * for eg: DrawMeshShader only binds set 3 using this while others bind all set this way
    */ 
    virtual void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const {}
    /*
    * Fills specialization constants for this shader
    */
    virtual void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const {}

    SharedPtr<ShaderCodeResource> getShaderCode(EShaderStage::Type shaderType) const;
    const std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>>& getShaders() const;
};