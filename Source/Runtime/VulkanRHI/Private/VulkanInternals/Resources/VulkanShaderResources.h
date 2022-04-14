/*!
 * \file VulkanShaderResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "IVulkanResources.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "ShaderReflected.h"
#include "VulkanInternals/VulkanMacros.h"

class VulkanShaderCodeResource final
    : public ShaderCodeResource
    , public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderCodeResource, , ShaderCodeResource, )
private:
    const ShaderStageDescription *stageDescription;

public:
    VkShaderModule shaderModule;

protected:
    VulkanShaderCodeResource();

public:
    VulkanShaderCodeResource(
        const String &shaderName, const ShaderStageDescription *desc, const uint8 *shaderCodePtr);
    /* IVulkanResources overrides */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    /* ShaderResource overrides */
    void reinitResources() override;
    void release() override;
    String getResourceName() const override;

    EShaderStage::Type shaderStage() const override;
    /* End overrides */

    const ShaderStageDescription &getStageDesc() const;
};

class VulkanShaderResource
    : public ShaderResource
    , public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanShaderResource, , ShaderResource, )
private:
    std::vector<uint8> shaderCode;
    ShaderReflected reflectedData;

protected:
    VulkanShaderResource() = default;

public:
    VulkanShaderResource(const ShaderConfigCollector *inConfig);

    /* IVulkanResources overrides */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override { return 0; }
    /* ShaderResource overrides */
    void init() final;
    const ShaderReflected *getReflection() const override;
    /* End overrides */
};