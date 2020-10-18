#pragma once
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "VulkanCommandBufferManager.h"

class IGraphicsInstance;
class VulkanDevice;

class VulkanCommandList : public IRenderCommandList
{
private:
    IGraphicsInstance* gInstance;
    VulkanDevice* vDevice;

    class VulkanCmdBufferManager* cmdBufferManager;

    FORCE_INLINE VkImageAspectFlags determineImageAspect(const ImageResource* image) const;
    // Determines mask that has info on how image can be access in pipelines
    FORCE_INLINE VkAccessFlags determineImageAccessMask(const ImageResource* image) const;
    // Determines the image layout if layout is yet to be defined
    FORCE_INLINE VkImageLayout determineImageLayout(const ImageResource* image) const;
    FORCE_INLINE VkImageLayout getImageLayout(const ImageResource* image) const;
public:

    VulkanCommandList(IGraphicsInstance* graphicsInstance, VulkanDevice* vulkanDevice);
    ~VulkanCommandList();

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) override;
    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) override;
    void copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies) override;

    void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) override;
    void copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) override;

    void setupInitialLayout(ImageResource* image) override;

    // Reusable here mean rerecord able command buffer
    const GraphicsResource* startCmd(String uniqueName, EQueueFunction queue, bool bIsReusable) override;
    void endCmd(const GraphicsResource* cmdBuffer) override;
    void freeCmd(const GraphicsResource* cmdBuffer) override;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo
        , const SharedPtr<GraphicsFence>& fence) override;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo) override;
    void waitIdle() override;

};