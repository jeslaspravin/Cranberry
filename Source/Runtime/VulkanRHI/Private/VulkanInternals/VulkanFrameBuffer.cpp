/*!
 * \file VulkanFrameBuffer.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <array>

#include "Logger/Logger.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "Resources/VulkanWindowCanvas.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanGraphicsInstance.h"
#include "VulkanGraphicsTypes.h"
#include "VulkanInternals/Resources/VulkanMemoryResources.h"
#include "VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "VulkanRHIModule.h"

//////////////////////////////////////////////////////////////////////////
//// Frame buffer and GBuffers related
//////////////////////////////////////////////////////////////////////////

struct VulkanFrameBuffer final : public Framebuffer
{
    VkFramebuffer frameBuffer = nullptr;

    ~VulkanFrameBuffer();
};

VulkanFrameBuffer::~VulkanFrameBuffer()
{
    VulkanGraphicsHelper::destroyFramebuffer(IVulkanRHIModule::get()->getGraphicsInstance(), frameBuffer);
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

VkFramebuffer VulkanGraphicsHelper::getFramebuffer(const struct Framebuffer *appFrameBuffer)
{
    return static_cast<const VulkanFrameBuffer *>(appFrameBuffer)->frameBuffer;
}

Framebuffer *VulkanGraphicsHelper::createFbInstance() const { return new VulkanFrameBuffer(); }

void VulkanGraphicsHelper::initializeFb(class IGraphicsInstance *graphicsInstance, Framebuffer *fb, const Size2D &frameSize) const
{
    auto *vulkanFb = static_cast<VulkanFrameBuffer *>(fb);
    VkRenderPass dummyRenderPass = VulkanGraphicsHelper::createDummyRenderPass(graphicsInstance, fb);

    std::vector<VkImageView> imageViews;
    imageViews.reserve(fb->textures.size());
    ImageViewInfo imageViewInfo;

    uint32 layers = (fb->textures.empty() || (fb->textures[0]->getImageSize().z == 1 && fb->textures[0]->getLayerCount() == 1))
                        ? 1
                        : fb->textures[0]->getLayerCount();
    // if texture is having more layers or 3D then we need view types
    int32 imgViewType = (layers == 1) ? -1 : VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    for (ImageResourceRef &imgRes : fb->textures)
    {
        imageViews.push_back(static_cast<VulkanImageResource *>(imgRes.reference())->getImageView(imageViewInfo, imgViewType));
    }

    FRAMEBUFFER_CREATE_INFO(fbCreateInfo);
    fbCreateInfo.renderPass = dummyRenderPass;
    fbCreateInfo.width = frameSize.x;
    fbCreateInfo.height = frameSize.y;
    fbCreateInfo.layers = layers;
    fbCreateInfo.attachmentCount = uint32(imageViews.size());
    fbCreateInfo.pAttachments = imageViews.data();

    if (vulkanFb->frameBuffer)
    {
        VulkanGraphicsHelper::destroyFramebuffer(graphicsInstance, vulkanFb->frameBuffer);
    }
    VulkanGraphicsHelper::createFramebuffer(graphicsInstance, fbCreateInfo, &vulkanFb->frameBuffer);
    VulkanGraphicsHelper::destroyRenderPass(graphicsInstance, dummyRenderPass);
}

void VulkanGraphicsHelper::initializeSwapchainFb(
    class IGraphicsInstance *graphicsInstance, Framebuffer *fb, WindowCanvasRef canvas, uint32 swapchainIdx
) const
{
    const auto *vulkanWindowCanvas = static_cast<const VulkanWindowCanvas *>(canvas.reference());
    ImageResourceRef dummyImageResource(new ImageResource(ImageResourceCreateInfo{ vulkanWindowCanvas->windowCanvasFormat() }));
    dummyImageResource->setResourceName(TCHAR("FB_DummyTexture_NoInit"));

    auto *vulkanFb = static_cast<VulkanFrameBuffer *>(fb);
    vulkanFb->textures.push_back(dummyImageResource);

    VkImageView swapchainImgView = vulkanWindowCanvas->swapchainImageView(swapchainIdx);

    VkRenderPass dummyRenderPass = VulkanGraphicsHelper::createDummyRenderPass(graphicsInstance, fb);
    FRAMEBUFFER_CREATE_INFO(fbCreateInfo);
    fbCreateInfo.attachmentCount = 1;
    fbCreateInfo.pAttachments = &swapchainImgView;
    fbCreateInfo.renderPass = dummyRenderPass;
    fbCreateInfo.width = canvas->imageSize().x;
    fbCreateInfo.height = canvas->imageSize().y;
    fbCreateInfo.layers = 1;

    if (vulkanFb->frameBuffer)
    {
        VulkanGraphicsHelper::destroyFramebuffer(graphicsInstance, vulkanFb->frameBuffer);
    }
    VulkanGraphicsHelper::createFramebuffer(graphicsInstance, fbCreateInfo, &vulkanFb->frameBuffer);
    VulkanGraphicsHelper::destroyRenderPass(graphicsInstance, dummyRenderPass);
    vulkanFb->textures.clear();
}

// Assumption made : that  we are never going to use input attachments so all texture except depth
// attachments will be used as color attachment only Assumption made : we only use one subpass to make
// things easier and also one subpass will not consider depth and preserve attachments for compatibility
// Assumption made : we are never going to use preserve attachments
VkRenderPass VulkanGraphicsHelper::createDummyRenderPass(class IGraphicsInstance *graphicsInstance, const struct Framebuffer *framebuffer)
{
    VkRenderPass renderPass;

    std::vector<VkAttachmentDescription> renderPassAttachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    std::vector<VkAttachmentReference> resolveAttachmentRefs;
    std::vector<VkAttachmentReference> depthAttachmentRef;

    renderPassAttachments.reserve((framebuffer->textures.size()));

    for (uint32 attachmentIdx = 0; attachmentIdx < framebuffer->textures.size();)
    {
        const ImageResourceRef &resource = framebuffer->textures[attachmentIdx];

        VkAttachmentDescription attachmentDesc;
        attachmentDesc.flags = 0;
        attachmentDesc.format = EngineToVulkanAPI::vulkanDataFormat(resource->imageFormat());
        attachmentDesc.samples = VkSampleCountFlagBits(resource->sampleCount());
        // Since only above two matters for dummy render pass
        attachmentDesc.loadOp = attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.storeOp = attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        renderPassAttachments.emplace_back(attachmentDesc);
        // Since there cannot be any resolve for depth texture as of Vulkan 1.2.135 we do not have
        // resolve attachment for depth
        if (EPixelDataFormat::isDepthFormat(resource->imageFormat()))
        {
            fatalAssertf(depthAttachmentRef.size() == 0, "More than one depth attachment is not allowed");
            depthAttachmentRef.push_back({ attachmentIdx, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
            ++attachmentIdx;
        }
        else
        {
            colorAttachmentRefs.push_back({ attachmentIdx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

            if (framebuffer->bHasResolves)
            {
                attachmentDesc.format = EngineToVulkanAPI::vulkanDataFormat(framebuffer->textures[attachmentIdx + 1]->imageFormat());
                attachmentDesc.samples = VkSampleCountFlagBits(framebuffer->textures[attachmentIdx + 1]->sampleCount());

                renderPassAttachments.emplace_back(attachmentDesc);
                resolveAttachmentRefs.push_back({ attachmentIdx + 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

                attachmentIdx += 2;
            }
            else
            {
                ++attachmentIdx;
            }
        }
    }

    VkSubpassDescription dummySubpass;
    dummySubpass.flags = 0;
    dummySubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    dummySubpass.colorAttachmentCount = uint32(colorAttachmentRefs.size());
    dummySubpass.pColorAttachments = colorAttachmentRefs.data();
    dummySubpass.pResolveAttachments = resolveAttachmentRefs.data();
    dummySubpass.inputAttachmentCount = 0;
    dummySubpass.pInputAttachments = nullptr;
    dummySubpass.preserveAttachmentCount = 0;
    dummySubpass.pPreserveAttachments = nullptr;
    dummySubpass.pDepthStencilAttachment = depthAttachmentRef.empty() ? nullptr : &depthAttachmentRef[0];

    // TODO(Jeslas) : Non parallel renderpass modify in future to support async passes.
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].dependencyFlags = dependencies[1].dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[0].dstSubpass = dependencies[1].srcSubpass = 0;
    dependencies[0].srcSubpass = dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].srcStageMask = dependencies[1].srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependencies[0].dstStageMask = dependencies[1].dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dependencies[0].srcAccessMask = dependencies[1].srcAccessMask = dependencies[0].dstAccessMask = dependencies[1].dstAccessMask = 0;

    RENDERPASS_CREATE_INFO(renderPassCreateInfo);
    renderPassCreateInfo.attachmentCount = uint32(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.dependencyCount = uint32(dependencies.size());
    renderPassCreateInfo.pDependencies = dependencies.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &dummySubpass;

    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    if (device->vkCreateRenderPass(device->logicalDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Failed creating render pass");
        renderPass = nullptr;
    }
    return renderPass;
}

VkRenderPass VulkanGraphicsHelper::createRenderPass(
    class IGraphicsInstance *graphicsInstance, const struct GenericRenderPassProperties &renderpassProps,
    const struct RenderPassAdditionalProps &additionalProps
)
{
    using namespace EAttachmentOp;
    fatalAssertf(
        !additionalProps.bUsedAsPresentSource
            || (renderpassProps.bOneRtPerFormat && renderpassProps.renderpassAttachmentFormat.attachments.size() == 1),
        "Presentable swapchain attachments cannot have more than one attachments or more than 1 sample "
        "count"
    );

    VkRenderPass renderPass;

    std::vector<VkAttachmentDescription> renderPassAttachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    std::vector<VkAttachmentReference> resolveAttachmentRefs;
    std::vector<VkAttachmentReference> depthAttachmentRef;

    bool bCanInitialLayoutBeUndef = additionalProps.bAllowUndefinedLayout && additionalProps.depthLoadOp != EAttachmentOp::LoadOp::Load
                                    && additionalProps.stencilLoadOp != EAttachmentOp::LoadOp::Load
                                    && additionalProps.colorAttachmentLoadOp != EAttachmentOp::LoadOp::Load;

    for (EPixelDataFormat::Type attachmentFormat : renderpassProps.renderpassAttachmentFormat.attachments)
    {
        VkAttachmentDescription attachmentDesc;
        attachmentDesc.flags = 0;
        attachmentDesc.format = EngineToVulkanAPI::vulkanDataFormat(attachmentFormat);
        attachmentDesc.samples = VkSampleCountFlagBits(renderpassProps.multisampleCount);
        attachmentDesc.stencilLoadOp = EngineToVulkanAPI::vulkanLoadOp(additionalProps.stencilLoadOp);
        attachmentDesc.stencilStoreOp = EngineToVulkanAPI::vulkanStoreOp(additionalProps.stencilStoreOp);

        // Since there cannot be any resolve for depth texture as of Vulkan 1.2.135 we do not have
        // resolve attachment for depth
        if (EPixelDataFormat::isDepthFormat(attachmentFormat))
        {
            fatalAssertf(depthAttachmentRef.size() == 0, "More than one depth attachment is not allowed");

            attachmentDesc.loadOp = EngineToVulkanAPI::vulkanLoadOp(additionalProps.depthLoadOp);
            attachmentDesc.storeOp = EngineToVulkanAPI::vulkanStoreOp(additionalProps.depthStoreOp);

            // Since depths are always same texture for both attachments and shader read
            attachmentDesc.initialLayout = attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentDesc.initialLayout = bCanInitialLayoutBeUndef ? VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED : attachmentDesc.initialLayout;

            depthAttachmentRef.push_back({ uint32(renderPassAttachments.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
            renderPassAttachments.push_back(attachmentDesc);
        }
        else
        {
            attachmentDesc.loadOp = EngineToVulkanAPI::vulkanLoadOp(additionalProps.colorAttachmentLoadOp);
            attachmentDesc.storeOp = EngineToVulkanAPI::vulkanStoreOp(additionalProps.colorAttachmentStoreOp);
            attachmentDesc.initialLayout = attachmentDesc.finalLayout
                = additionalProps.bUsedAsPresentSource ? VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                  : renderpassProps.bOneRtPerFormat    ? VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                       : VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDesc.initialLayout = bCanInitialLayoutBeUndef ? VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED : attachmentDesc.initialLayout;

            colorAttachmentRefs.push_back({ uint32(renderPassAttachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
            renderPassAttachments.push_back(attachmentDesc);

            if (!renderpassProps.bOneRtPerFormat)
            {
                // Since resolve attachments(Shader read only) always have 1 samples
                attachmentDesc.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

                // LOAD and STORE assumed it is right choice
                attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

                // Since resolve attachment has to be shader read only before(if required by
                // default) and after pass
                attachmentDesc.initialLayout = attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                attachmentDesc.initialLayout
                    = bCanInitialLayoutBeUndef ? VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED : attachmentDesc.initialLayout;

                resolveAttachmentRefs.push_back({ uint32(renderPassAttachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                renderPassAttachments.push_back(attachmentDesc);
            }
        }
    }

    VkSubpassDescription dummySubpass;
    dummySubpass.flags = 0;
    dummySubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    dummySubpass.colorAttachmentCount = uint32(colorAttachmentRefs.size());
    dummySubpass.pColorAttachments = colorAttachmentRefs.data();
    dummySubpass.pResolveAttachments = resolveAttachmentRefs.data();
    dummySubpass.inputAttachmentCount = 0;
    dummySubpass.pInputAttachments = nullptr;
    dummySubpass.preserveAttachmentCount = 0;
    dummySubpass.pPreserveAttachments = nullptr;
    dummySubpass.pDepthStencilAttachment = depthAttachmentRef.empty() ? nullptr : &depthAttachmentRef[0];

    // TODO(Jeslas) : Non parallel renderpass modify in future to support async passes.
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].dependencyFlags = dependencies[1].dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[0].dstSubpass = dependencies[1].srcSubpass = 0;
    dependencies[0].srcSubpass = dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].srcStageMask = dependencies[1].srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependencies[0].dstStageMask = dependencies[1].dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dependencies[0].srcAccessMask = dependencies[1].srcAccessMask = dependencies[0].dstAccessMask = dependencies[1].dstAccessMask = 0;

    RENDERPASS_CREATE_INFO(renderPassCreateInfo);
    renderPassCreateInfo.attachmentCount = uint32(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.dependencyCount = uint32(dependencies.size());
    renderPassCreateInfo.pDependencies = dependencies.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &dummySubpass;

    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    if (device->vkCreateRenderPass(device->logicalDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Failed creating render pass");
        renderPass = nullptr;
    }
    return renderPass;
}