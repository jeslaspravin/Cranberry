#include "IRenderCommandList.h"
#include "RenderingContexts.h"
#include "../GraphicsHelper.h"
#include "../../Core/Types/Colors.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Platform/PlatformFunctions.h"
#include "../ShaderCore/ShaderParameterResources.h"

#include <utility>

ScopedCommandMarker::ScopedCommandMarker(const class IRenderCommandList* commandList, const GraphicsResource* commandBuffer, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/)
    : cmdList(commandList)
    , cmdBuffer(commandBuffer)
{
    cmdList->cmdBeginBufferMarker(cmdBuffer, name, color);
}

ScopedCommandMarker::~ScopedCommandMarker()
{
    cmdList->cmdEndBufferMarker(cmdBuffer);
}

class RenderCommandList final : public IRenderCommandList
{
private:
    IRenderCommandList* cmdList;

public:
    void setup(IRenderCommandList* commandList) override;

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) override;
    void copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies) override;
    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) override;

    void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) override;
    void copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) override;

    void setupInitialLayout(ImageResource* image) override;

    void cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor) const override;
    void cmdEndRenderPass(const GraphicsResource* cmdBuffer) const override;

    void cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const override;
    void cmdBindDescriptorsSetInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::map<uint32, const ShaderParameters*>& descriptorsSets) const override;
    void cmdBindDescriptorsSetsInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const override;
    void cmdBindVertexBuffers(const GraphicsResource* cmdBuffer, uint32 firstBinding, const std::vector<const BufferResource*>& vertexBuffers, const std::vector<uint64>& offsets) const override;
    void cmdBindIndexBuffer(const GraphicsResource* cmdBuffer, const BufferResource* indexBuffer, uint64 offset = 0) const override;

    void cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance = 0, uint32 instanceCount = 1, int32 vertexOffset = 0) const override;

    void cmdSetViewportAndScissors(const GraphicsResource* cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>>& viewportAndScissors, uint32 firstViewport = 0) const override;
    void cmdSetViewportAndScissor(const GraphicsResource* cmdBuffer, const QuantizedBox2D& viewport, const QuantizedBox2D& scissor, uint32 atViewport = 0) const override;

    void cmdBeginBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const override;
    void cmdInsertBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const override;
    void cmdEndBufferMarker(const GraphicsResource* commandBuffer) const override;

    const GraphicsResource* startCmd(const String& uniqueName, EQueueFunction queue, bool bIsReusable) override;
    void endCmd(const GraphicsResource* cmdBuffer) override;
    void freeCmd(const GraphicsResource* cmdBuffer) override;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo
        , const SharedPtr<GraphicsFence>& fence) override;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo) override;
    void finishCmd(const GraphicsResource* cmdBuffer) override;
    void finishCmd(const String& uniqueName) override;
    const GraphicsResource* getCmdBuffer(const String& uniqueName) const override;
    void waitIdle() override;

};

void RenderCommandList::cmdBindDescriptorsSetInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::map<uint32, const ShaderParameters*>& descriptorsSets) const
{
    cmdList->cmdBindDescriptorsSetInternal(cmdBuffer, contextPipeline, descriptorsSets);
}

void RenderCommandList::cmdBindDescriptorsSetsInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const
{
    cmdList->cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline, descriptorsSets);
}

void RenderCommandList::cmdBindVertexBuffers(const GraphicsResource* cmdBuffer, uint32 firstBinding, const std::vector<const BufferResource*>& vertexBuffers, const std::vector<uint64>& offsets) const
{
    cmdList->cmdBindVertexBuffers(cmdBuffer, firstBinding, vertexBuffers, offsets);
}

void RenderCommandList::cmdBindIndexBuffer(const GraphicsResource* cmdBuffer, const BufferResource* indexBuffer, uint64 offset /*= 0*/) const
{
    cmdList->cmdBindIndexBuffer(cmdBuffer, indexBuffer, offset);
}

void RenderCommandList::cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/, int32 vertexOffset /*= 0*/) const
{
    cmdList->cmdDrawIndexed(cmdBuffer, firstIndex, indexCount, firstInstance, instanceCount, vertexOffset);
}

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

const GraphicsResource* RenderCommandList::startCmd(const String& uniqueName, EQueueFunction queue, bool bIsReusable)
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

void RenderCommandList::finishCmd(const GraphicsResource* cmdBuffer)
{
    cmdList->finishCmd(cmdBuffer);
}

void RenderCommandList::finishCmd(const String& uniqueName)
{
    cmdList->finishCmd(uniqueName);
}

const GraphicsResource* RenderCommandList::getCmdBuffer(const String& uniqueName) const
{
    return cmdList->getCmdBuffer(uniqueName);
}

void RenderCommandList::waitIdle()
{
    cmdList->waitIdle();
}

void RenderCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo)
{
    cmdList->copyToImage(dst, pixelData, copyInfo);
}

void RenderCommandList::copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo)
{
    cmdList->copyOrResolveImage(src, dst, srcInfo, dstInfo);
}

void RenderCommandList::setupInitialLayout(ImageResource* image)
{
    cmdList->setupInitialLayout(image);
}

void RenderCommandList::cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor) const
{
    cmdList->cmdBeginRenderPass(cmdBuffer, contextPipeline, renderArea, renderpassAdditionalProps, clearColor);
}

void RenderCommandList::cmdEndRenderPass(const GraphicsResource* cmdBuffer) const
{
    cmdList->cmdEndRenderPass(cmdBuffer);
}

void RenderCommandList::cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const
{
    cmdList->cmdBindGraphicsPipeline(cmdBuffer, contextPipeline, state);
}

void RenderCommandList::cmdSetViewportAndScissors(const GraphicsResource* cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>>& viewportAndScissors, uint32 firstViewport /*= 0*/) const
{
    cmdList->cmdSetViewportAndScissors(cmdBuffer, viewportAndScissors, firstViewport);
}

void RenderCommandList::cmdSetViewportAndScissor(const GraphicsResource* cmdBuffer, const QuantizedBox2D& viewport, const QuantizedBox2D& scissor, uint32 atViewport /*= 0*/) const
{
    cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor, atViewport);
}

void RenderCommandList::cmdBeginBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    cmdList->cmdBeginBufferMarker(commandBuffer, name, color);
}

void RenderCommandList::cmdInsertBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    cmdList->cmdInsertBufferMarker(commandBuffer, name, color);
}

void RenderCommandList::cmdEndBufferMarker(const GraphicsResource* commandBuffer) const
{
    cmdList->cmdEndBufferMarker(commandBuffer);
}

void IRenderCommandList::copyPixelsTo(BufferResource* stagingBuffer, uint8* stagingPtr, const std::vector<Color>& pixelData
    , const EPixelDataFormat::PixelFormatInfo* formatInfo) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    debugAssert(colorCompBits == 8);

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());

    if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
    {
        // Copying data
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
    else
    {
        fatalAssert(false, "Big endian platform not supported yet");
    }
}

void IRenderCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        Logger::error("VulkanCommandList", "%s() : Texel data count is not sufficient to fill all texels of %s", __func__
            , dst->getResourceName().getChar());
        return;
    }
    CopyPixelsToImageInfo copyInfo;
    copyInfo.dstOffset = { 0,0,0 };
    copyInfo.extent = dst->getImageSize();
    copyInfo.subres.baseLayer = 0;
    copyInfo.subres.layersCount = dst->getLayerCount();
    copyInfo.subres.baseMip = 0;
    copyInfo.subres.mipCount = dst->getNumOfMips();
    copyInfo.bGenerateMips = true;
    copyInfo.mipFiltering = ESamplerFiltering::Nearest;
    copyToImage(dst, pixelData, copyInfo);
}

void IRenderCommandList::cmdBindDescriptorsSets(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const ShaderParameters* descriptorsSets) const
{
    if (descriptorsSets->getParamLayout()->getType()->isChildOf<ShaderParametersLayout>())
    {
        cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline.getPipeline(), { descriptorsSets });
    }
    else if(descriptorsSets->getParamLayout()->getType()->isChildOf<ShaderSetParametersLayout>())
    {
        std::pair<uint32, const ShaderParameters*> setIdToDescsSet{ 
            static_cast<const ShaderSetParametersLayout*>(descriptorsSets->getParamLayout())->getSetID()
            , descriptorsSets };
        cmdBindDescriptorsSetInternal(cmdBuffer, contextPipeline.getPipeline(), { setIdToDescsSet });
    }
}

void IRenderCommandList::cmdBindDescriptorsSets(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const
{
    std::vector<const ShaderParameters*> shaderParamsSetsList;
    std::map<uint32, const ShaderParameters*> shaderParamsSetList;

    for (const ShaderParameters* shaderParams : descriptorsSets)
    {
        if (shaderParams->getParamLayout()->getType()->isChildOf<ShaderParametersLayout>())
        {
            shaderParamsSetsList.emplace_back(shaderParams);
        }
        else if (shaderParams->getParamLayout()->getType()->isChildOf<ShaderSetParametersLayout>())
        {
            shaderParamsSetList[static_cast<const ShaderSetParametersLayout*>(shaderParams->getParamLayout())->getSetID()] = shaderParams;
        }
    }

    if (!shaderParamsSetsList.empty())
    {
        cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline.getPipeline(), shaderParamsSetsList);
    }

    if (!shaderParamsSetList.empty())
    {
        cmdBindDescriptorsSetInternal(cmdBuffer, contextPipeline.getPipeline(), shaderParamsSetList);
    }
}

IRenderCommandList* IRenderCommandList::genericInstance()
{
    return new RenderCommandList();
}