#include "../../RenderApi/GBuffersAndTextures.h"
#include "../VulkanGraphicsHelper.h"
#include "Resources/VulkanMemoryResources.h"
#include "../../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../../Core/Engine/GameEngine.h"
#include "../VulkanGraphicsInstance.h"
#include "../../Core/Logger/Logger.h"
#include "Resources/VulkanWindowCanvas.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"


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
    VulkanGraphicsHelper::destroyFramebuffer(gEngine->getRenderApi()->getGraphicsInstance(), frameBuffer);
}

#if RENDERAPI_VULKAN

Framebuffer* GBuffers::createFbInternal()
{
    return new VulkanFrameBuffer();
}

void GBuffers::initializeInternal(Framebuffer* fb, const Size2D& frameSize)
{
    IGraphicsInstance* gInstance = gEngine->getRenderApi()->getGraphicsInstance();
    auto* vulkanFb = static_cast<VulkanFrameBuffer*>(fb);
    VkRenderPass dummyRenderPass = VulkanGraphicsHelper::createDummyRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), fb);

    std::vector<VkImageView> imageViews;
    imageViews.reserve(fb->textures.size());
    ImageViewInfo imageViewInfo;
    for (ImageResource* imgRes : fb->textures)
    {
        imageViews.push_back(static_cast<VulkanImageResource*>(imgRes)->getImageView(imageViewInfo));
    }

    FRAMEBUFFER_CREATE_INFO(fbCreateInfo);
    fbCreateInfo.renderPass = dummyRenderPass;
    fbCreateInfo.width = frameSize.x;
    fbCreateInfo.height = frameSize.y;
    fbCreateInfo.layers = 1;
    fbCreateInfo.attachmentCount = uint32(imageViews.size());
    fbCreateInfo.pAttachments = imageViews.data();

    if (vulkanFb->frameBuffer)
    {
        VulkanGraphicsHelper::destroyFramebuffer(gInstance, vulkanFb->frameBuffer);
    }
    VulkanGraphicsHelper::createFramebuffer(gInstance, fbCreateInfo, &vulkanFb->frameBuffer);
    VulkanGraphicsHelper::destroyRenderPass(gInstance, dummyRenderPass);
}

void GBuffers::initializeSwapchainFb(Framebuffer* fb, const class GenericWindowCanvas* canvas, const Size2D& frameSize, uint32 swapchainIdx)
{
    IGraphicsInstance* gInstance = gEngine->getRenderApi()->getGraphicsInstance();
    const auto* vulkanWindowCanvas = static_cast<const VulkanWindowCanvas*>(canvas);
    ImageResource dummyImageResource(vulkanWindowCanvas->windowCanvasFormat());

    auto* vulkanFb = static_cast<VulkanFrameBuffer*>(fb);
    vulkanFb->textures.push_back(&dummyImageResource);

    VkImageView swapchainImgView = vulkanWindowCanvas->swapchainImageView(swapchainIdx);

    VkRenderPass dummyRenderPass = VulkanGraphicsHelper::createDummyRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), fb);
    FRAMEBUFFER_CREATE_INFO(fbCreateInfo);
    fbCreateInfo.attachmentCount = 1;
    fbCreateInfo.pAttachments = &swapchainImgView;
    fbCreateInfo.renderPass = dummyRenderPass;
    fbCreateInfo.width = frameSize.x;
    fbCreateInfo.height = frameSize.y;
    fbCreateInfo.layers = 1;

    if (vulkanFb->frameBuffer)
    {
        VulkanGraphicsHelper::destroyFramebuffer(gInstance, vulkanFb->frameBuffer);
    }
    VulkanGraphicsHelper::createFramebuffer(gInstance, fbCreateInfo, &vulkanFb->frameBuffer);
    VulkanGraphicsHelper::destroyRenderPass(gInstance, dummyRenderPass);
    vulkanFb->textures.clear();
}

#endif//RENDERAPI_VULKAN

VkFramebuffer VulkanGraphicsHelper::getFramebuffer(struct Framebuffer* appFrameBuffer)
{
    return static_cast<VulkanFrameBuffer*>(appFrameBuffer)->frameBuffer;
}

// Assumption made : that  we are never going to use input attachments so all texture except depth attachments will be used as color attachment only
// Assumption made : we only use one subpass to make things easier and also one subpass will not consider depth and preserve attachments for compatibility
// Assumption made : we are never going to use resolve or preserve attachments
VkRenderPass VulkanGraphicsHelper::createDummyRenderPass(class IGraphicsInstance* graphicsInstance, const struct Framebuffer* framebuffer)
{
    VkRenderPass renderPass;

    std::vector<VkAttachmentDescription> renderPassAttachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    std::vector<VkAttachmentReference> resolveAttachmentRefs;
    std::vector<VkAttachmentReference> depthAttachmentRef;

    renderPassAttachments.reserve((framebuffer->textures.size()));

    for (uint32 attachmentIdx = 0; attachmentIdx < framebuffer->textures.size();)
    {
        const ImageResource* resource = framebuffer->textures[attachmentIdx];

        VkAttachmentDescription attachmentDesc;
        attachmentDesc.flags = 0;
        attachmentDesc.format = VkFormat(EPixelDataFormat::getFormatInfo(resource->imageFormat())->format);
        attachmentDesc.samples = VkSampleCountFlagBits(resource->sampleCount());
        // Since only above two matters for dummy render pass
        attachmentDesc.loadOp = attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.storeOp = attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        renderPassAttachments.push_back(attachmentDesc);
        // Since there cannot be any resolve for depth texture as of Vulkan 1.2.135 we do not have resolve attachment for depth
        if (EPixelDataFormat::isDepthFormat(resource->imageFormat()))
        {
            fatalAssert(depthAttachmentRef.size() == 0, "More than one depth attachment is not allowed");
            depthAttachmentRef.push_back({ attachmentIdx , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
            ++attachmentIdx;
        }
        else
        {
            colorAttachmentRefs.push_back({ attachmentIdx , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

            if (framebuffer->bHasResolves)
            {
                attachmentDesc.format = VkFormat(EPixelDataFormat::getFormatInfo(framebuffer->textures[attachmentIdx + 1]->imageFormat())->format);
                attachmentDesc.samples = VkSampleCountFlagBits(framebuffer->textures[attachmentIdx + 1]->sampleCount());

                renderPassAttachments.push_back(attachmentDesc);
                resolveAttachmentRefs.push_back({ attachmentIdx + 1 , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

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
    dummySubpass.pDepthStencilAttachment = depthAttachmentRef.size() != 0 ? &depthAttachmentRef[0] : nullptr;

    RENDERPASS_CREATE_INFO(renderPassCreateInfo);
    renderPassCreateInfo.attachmentCount = uint32(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = nullptr;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &dummySubpass;

    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    if (device->vkCreateRenderPass(device->logicalDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        Logger::error("VulkanGraphicsHelper", "%s() : Failed creating render pass", __func__);
        renderPass = nullptr;
    }
    return renderPass;
}