/*!
 * \file VulkanRenderingContexts.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderApi/Rendering/RenderingContexts.h"
#include "RenderInterface/Resources/DeferredDeleter.h"

#include <vulkan/vulkan_core.h>

class DrawMeshShaderConfig;

class VulkanGlobalRenderingContext final : public GlobalRenderingContextBase
{
private:
    using RenderpassPropsPair = std::pair<RenderPassAdditionalProps, VkRenderPass>;

    // Render passes(different layout or load store op) for each variant of pipeline compatible render
    // pass attachments and sub passes
    std::unordered_map<GenericRenderPassProperties, std::vector<RenderpassPropsPair>> genericRenderPasses;
    std::unordered_map<ERenderPassFormat::Type, std::vector<RenderpassPropsPair>> gbufferRenderPasses;

    std::unordered_map<const ShaderResource *, VkPipelineLayout> pipelineLayouts;

#if DEFER_DELETION
    DeferredDeleter resourceDeleter;
#endif
private:
    VkRenderPass createGbufferRenderpass(ERenderPassFormat::Type rpUsageFormat, const RenderPassAdditionalProps &additionalProps) const;

protected:
    /* GlobalRenderingContextBase overrides */
    void initApiInstances() final;
    void initializeApiContext() final;
    void clearApiContext() final;
    void initializeGenericGraphicsPipeline(PipelineBase *pipeline) final;

    /* Override ends */

public:
    VkRenderPass getRenderPass(ERenderPassFormat::Type renderpassFormat, const RenderPassAdditionalProps &additionalProps);
    VkRenderPass getRenderPass(const GenericRenderPassProperties &renderpassProps, const RenderPassAdditionalProps &additionalProps);

#if DEFER_DELETION
    FORCE_INLINE DeferredDeleter *getDeferredDeleter() { return &resourceDeleter; }
#endif
};