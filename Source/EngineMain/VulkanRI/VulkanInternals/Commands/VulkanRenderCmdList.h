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
public:

    VulkanCommandList(IGraphicsInstance* graphicsInstance, VulkanDevice* vulkanDevice);
    ~VulkanCommandList();

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) override;
    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) override;
};