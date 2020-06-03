#include "IRenderCommandList.h"
#include "../GraphicsHelper.h"
#include "../Resources/MemoryResources.h"
#include "../../Core/Types/Colors.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Types/CoreDefines.h"

#include <utility>

class RenderCommandList final : public IRenderCommandList
{
private:
    IRenderCommandList* cmdList;

public:
    void setup(IRenderCommandList* commandList) override;

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) override;
    void copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies) override;
    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) override;

    void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyImageInfo& copyInfo) override;
    void copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& copyInfo) override;

    const GraphicsResource* startCmd(String uniqueName, EQueueFunction queue, bool bIsReusable) override;
    void endCmd(const GraphicsResource* cmdBuffer) override;
    void freeCmd(const GraphicsResource* cmdBuffer) override;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo
        , const SharedPtr<GraphicsFence>& fence) override;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo) override;
    void waitIdle() override;
};

void RenderCommandList::setup(IRenderCommandList* commandList)
{
    if (commandList != cmdList)
    {
        cmdList = commandList;
    }
}

void RenderCommandList::copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo)
{
    cmdList->copyBuffer(src, dst, copyInfo);
}

void RenderCommandList::copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size)
{
    cmdList->copyToBuffer(dst, dstOffset, dataToCopy, size);
}

void RenderCommandList::copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies)
{
    cmdList->copyToBuffer(batchCopies);
}

const GraphicsResource* RenderCommandList::startCmd(String uniqueName, EQueueFunction queue, bool bIsReusable)
{
    return cmdList->startCmd(uniqueName, queue, bIsReusable);
}

void RenderCommandList::endCmd(const GraphicsResource* cmdBuffer)
{
    cmdList->endCmd(cmdBuffer);
}

void RenderCommandList::freeCmd(const GraphicsResource* cmdBuffer)
{
    cmdList->freeCmd(cmdBuffer);
}

void RenderCommandList::submitCmd(EQueuePriority::Enum priority
    , const CommandSubmitInfo& submitInfo, const SharedPtr<GraphicsFence>& fence)
{
    cmdList->submitCmd(priority, submitInfo, fence);
}

void RenderCommandList::submitWaitCmd(EQueuePriority::Enum priority
    , const CommandSubmitInfo& submitInfo)
{
    cmdList->submitWaitCmd(priority, submitInfo);
}

void RenderCommandList::waitIdle()
{
    cmdList->waitIdle();
}

void RenderCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyImageInfo& copyInfo)
{
    cmdList->copyToImage(dst, pixelData, copyInfo);
}

void RenderCommandList::copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& copyInfo)
{
    cmdList->copyOrResolveImage(src, dst, copyInfo);
}

#if LITTLE_ENDIAN
void IRenderCommandList::copyPixelsTo(BufferResource* stagingBuffer, uint8* stagingPtr, const std::vector<Color>& pixelData
    , const EPixelDataFormat::PixelFormatInfo* formatInfo) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    debugAssert(colorCompBits == 8);

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());

    {// Copying data
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8* pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            for (uint8 compIdx = 0; compIdx < formatInfo->componentCount; ++compIdx)
            {
                const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));
                uint8* offsetStagingPtr = pixelStagingPtr + (compOffset / colorCompBits);

                // Left shift
                const uint32 byte1Shift = compOffset % colorCompBits;
                const uint8 byte1Mask = 0xFF << byte1Shift;
                *offsetStagingPtr |= (byte1Mask & (pixelData[i].getColorValue()[compIdx] << byte1Shift));

                if (byte1Shift > 0)
                {
                    const uint8 byte2Mask = ~byte1Mask;
                    *(offsetStagingPtr + 1) |= (byte2Mask & (pixelData[i].getColorValue()[compIdx] >> (colorCompBits - byte1Shift)));
                }
            }
        }
    }
}
#elif BIG_ENDIAN
static_assert(false, "Not supported endian platform");
#endif

void IRenderCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        Logger::error("VulkanCommandList", "%s() : Texel data count is not sufficient to fill all texels of %s", __func__
            , dst->getResourceName().getChar());
        return;
    }
    CopyImageInfo copyInfo;
    copyInfo.dstOffset = { 0,0,0 };
    copyInfo.extent = dst->getImageSize();
    copyInfo.layerBase = 0;
    copyInfo.layerCount = dst->getLayerCount();
    copyInfo.mipBase = 0;
    copyInfo.mipCount = dst->getNumOfMips();
    copyInfo.bGenerateMips = true;
    copyInfo.mipFiltering = ESamplerFiltering::Nearest;
    copyToImage(dst, pixelData, copyInfo);
}

IRenderCommandList* IRenderCommandList::genericInstance()
{
    return new RenderCommandList();
}