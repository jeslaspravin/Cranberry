/*!
 * \file ShaderResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Reflections/Functions.h"
#include "RenderInterface/Resources/GraphicsResources.h"
#include "RenderInterface/ShaderCore/ShaderInputOutput.h"
#include "String/String.h"

#include <map>

struct SpecializationConstantEntry;
class StringID;

namespace EShaderStage
{
enum Type
{
    Compute = 0,
    Vertex,
    TessellationControl,
    TessellatonEvaluate,
    Geometry,
    Fragment,
    ShaderStageMax
};

struct ShaderStageInfo
{
    const String name;
    const String shortName;
    // TODO(Jeslas) : Entry point is obtained from reflection data right now, so do I need here?
    const String entryPointName;
};

ENGINERENDERER_EXPORT const ShaderStageInfo *getShaderStageInfo(EShaderStage::Type shaderStage);
} // namespace EShaderStage

// Shader subresource for ShaderResource
class ENGINERENDERER_EXPORT ShaderCodeResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderCodeResource, , GraphicsResource, )

private:
    String shaderFileName;
    // Why multibyte here? As we need a AChar pointer to shader entry name when creating pipeline stages
    std::string shaderEntryPoint;

protected:
    const uint8 *shaderCode;

    ShaderCodeResource() = default;

public:
    ShaderCodeResource(const String &shaderName, const std::string &entryPointName, const uint8 *shaderCodePtr);

    /* GraphicsResources overrides */
    void init() override;
    String getResourceName() const override;
    void setResourceName(const String &) override {}
    /* End overrides */

    const std::string &entryPoint() const;
    virtual EShaderStage::Type shaderStage() const;
};

using SpecConstantNamedMap = std::map<StringID, SpecializationConstantEntry>;

// Collects shader information and feed it into creating ShaderResource
class ENGINERENDERER_EXPORT ShaderConfigCollector : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderConfigCollector, , GraphicsResource, )

private:
    String shaderName;
    class ShaderResource *shaderConfigured;

protected:
    ShaderConfigCollector() = default;

public:
    ShaderConfigCollector(const String &name);

    String getResourceName() const final { return shaderName; }
    struct ShaderReflected const *getReflection() const;

    // Provides file name from which the shaders have to be loaded
    virtual String getShaderFileName() const;
    /*
     * Binds BufferParamInfo for each buffer descriptor(Depending upon override corresponding set varies)
     * for eg: DrawMeshShader only binds set 3 using this while others bind all set this way
     */
    virtual void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &) const {}
    /*
     * Fills specialization constants for this shader
     */
    virtual void getSpecializationConsts(SpecConstantNamedMap &) const {}

    void setShaderConfigured(class ShaderResource *shaderResource) { shaderConfigured = shaderResource; }
};

class ENGINERENDERER_EXPORT ShaderResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderResource, , GraphicsResource, )

protected:
    const String SHADER_EXTENSION{ TCHAR("shader") };
    const String REFLECTION_EXTENSION{ TCHAR("ref") };

    const ShaderConfigCollector *shaderConfig;

protected:
    std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>> shaders;

    ShaderResource() = default;

public:
    ShaderResource(const ShaderConfigCollector *inConfig);

    /* GraphicsResource overrides */
    void init() override;
    void reinitResources() override;
    void release() override;

    String getResourceName() const final;
    void setResourceName(const String &) final {}

    /* End overrides */

    virtual struct ShaderReflected const *getReflection() const { return nullptr; }

    const ShaderConfigCollector *getShaderConfig() const { return shaderConfig; }
    /*
     * Binds BufferParamInfo for each buffer descriptor(Depending upon override corresponding set varies)
     * for eg: DrawMeshShader only binds set 3 using this while others bind all set this way
     */
    void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &bindingBuffers) const;
    /*
     * Fills specialization constants for this shader
     */
    void getSpecializationConsts(SpecConstantNamedMap &specializationConst) const;

    SharedPtr<ShaderCodeResource> getShaderCode(EShaderStage::Type shaderType) const;
    const std::map<EShaderStage::Type, SharedPtr<ShaderCodeResource>> &getShaders() const;
};