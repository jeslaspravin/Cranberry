#include "../../RenderApi/GBuffersAndTextures.h"
#include "../VulkanGraphicsHelper.h"
#include "Resources/VulkanMemoryResources.h"
#include "../../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../../Core/Engine/GameEngine.h"
#include "../VulkanGraphicsInstance.h"
#include "../../Core/Logger/Logger.h"


//////////////////////////////////////////////////////////////////////////
//// Frame buffer and GBuffers related
//////////////////////////////////////////////////////////////////////////

struct VulkanFrameBuffer final : public Framebuffer
{
    VkFramebuffer frameBuffer;

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

void GBuffers::initializeInternal(Framebuffer* fb)
{
    auto* vulkanFb = static_cast<VulkanFrameBuffer*>(fb);
    VkRenderPass dummyRenderPass = VulkanGraphicsHelper::createDummyRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), fb);

    std::vector<VkImageView> imageViews;
    imageViews.reserve(fb->textures.size());
    ImageViewInfo imageViewInfo;
    for (auto* rtTexture : fb->textures)
    {
        imageViews.push_back(static_cast<VulkanImageResource*>(fb->getImageResource(rtTexture))->getImageView(imageViewInfo));
    }

    FRAMEBUFFER_CREATE_INFO(FbCreateInfo);
    FbCreateInfo.renderPass = dummyRenderPass;
    FbCreateInfo.width = EngineSettings::screenSize.get().x;
    FbCreateInfo.height = EngineSettings::screenSize.get().y;
    FbCreateInfo.layers = 1;
    FbCreateInfo.attachmentCount = uint32(imageViews.size());
    FbCreateInfo.pAttachments = imageViews.data();

    VulkanGraphicsHelper::createFramebuffer(gEngine->getRenderApi()->getGraphicsInstance(), FbCreateInfo, &vulkanFb->frameBuffer);
    VulkanGraphicsHelper::destroyRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), dummyRenderPass);
}

#endif//RENDERAPI_VULKAN

VkFramebuffer VulkanGraphicsHelper::getFramebuffer(struct Framebuffer* appFrameBuffer)
{
    return static_cast<VulkanFrameBuffer*>(appFrameBuffer)->frameBuffer;
}

VkRenderPass VulkanGraphicsHelper::createDummyRenderPass(class IGraphicsInstance* graphicsInstance, const struct Framebuffer* framebuffer)
{
    VkRenderPass renderPass;

    std::vector<VkAttachmentDescription> renderPassAttachments;
    renderPassAttachments.reserve((framebuffer->textures.size()));

    for (const auto* rtTexture : framebuffer->textures)
    {
        const ImageResource* resource = framebuffer->getImageResource(rtTexture);
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
    }

    VkSubpassDescription dummySubpass;
    dummySubpass.flags = 0;
    dummySubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    dummySubpass.colorAttachmentCount = 0;
    dummySubpass.pColorAttachments = nullptr;
    dummySubpass.pResolveAttachments = nullptr;
    dummySubpass.inputAttachmentCount = 0;
    dummySubpass.pInputAttachments = nullptr;
    dummySubpass.preserveAttachmentCount = 0;
    dummySubpass.pPreserveAttachments = nullptr;
    dummySubpass.pDepthStencilAttachment = nullptr;

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