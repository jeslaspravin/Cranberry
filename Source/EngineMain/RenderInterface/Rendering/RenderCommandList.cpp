#include "IRenderCommandList.h"
#include "RenderingContexts.h"
#include "../GraphicsHelper.h"
#include "../../Core/Types/Colors.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Platform/PlatformFunctions.h"
#include "../ShaderCore/ShaderParameterResources.h"
#include "../Resources/ShaderResources.h"
#include "ShaderReflected.h"
#include "../../Core/Math/CoreMathTypes.h"

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
    void newFrame() override;

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) override;
    void copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies) override;
    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) override;

    void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) override;
    void copyToImage(ImageResource* dst, const std::vector<class LinearColor>& pixelData, const CopyPixelsToImageInfo& copyInfo) override;
    void copyToImageLinearMapped(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) override;

    void copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) override;

    void setupInitialLayout(ImageResource* image) override;

    void presentImage(const std::vector<class GenericWindowCanvas*>& canvases,
        const std::vector<uint32>& imageIndices, const std::vector<SharedPtr<class GraphicsSemaphore>>& waitOnSemaphores) override;

    void cmdBarrierResources(const GraphicsResource* cmdBuffer, const std::set<const ShaderParameters*>& descriptorsSets) override;

    void cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor) override;
    void cmdEndRenderPass(const GraphicsResource* cmdBuffer) override;

    void cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const override;
    void cmdBindComputePipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline) const override;
    void cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, uint32 stagesUsed, const uint8* data, const std::vector<CopyBufferInfo>& pushConsts) const override;
    void cmdBindDescriptorsSetInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::map<uint32, const ShaderParameters*>& descriptorsSets) const override;
    void cmdBindDescriptorsSetsInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const override;
    void cmdBindVertexBuffers(const GraphicsResource* cmdBuffer, uint32 firstBinding, const std::vector<const BufferResource*>& vertexBuffers, const std::vector<uint64>& offsets) const override;
    void cmdBindIndexBuffer(const GraphicsResource* cmdBuffer, const BufferResource* indexBuffer, uint64 offset = 0) const override;

    void cmdDispatch(const GraphicsResource* cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ = 1) const override;
    void cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance = 0, uint32 instanceCount = 1, int32 vertexOffset = 0) const override;
    void cmdDrawVertices(const GraphicsResource* cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance = 0, uint32 instanceCount = 1) const override;

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
    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands) override;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command) override;
    void finishCmd(const GraphicsResource* cmdBuffer) override;
    void finishCmd(const String& uniqueName) override;
    const GraphicsResource* getCmdBuffer(const String& uniqueName) const override;
    void waitIdle() override;
};


void RenderCommandList::cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, uint32 stagesUsed, const uint8* data, const std::vector<CopyBufferInfo>& pushConsts) const
{
    cmdList->cmdPushConstants(cmdBuffer, contextPipeline, stagesUsed, data, pushConsts);
}

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

void RenderCommandList::cmdDispatch(const GraphicsResource* cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ /*= 1*/) const
{
    cmdList->cmdDispatch(cmdBuffer, groupSizeX, groupSizeY, groupSizeZ);
}

void RenderCommandList::cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/, int32 vertexOffset /*= 0*/) const
{
    cmdList->cmdDrawIndexed(cmdBuffer, firstIndex, indexCount, firstInstance, instanceCount, vertexOffset);
}

void RenderCommandList::cmdDrawVertices(const GraphicsResource* cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/) const
{
    cmdList->cmdDrawVertices(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

void RenderCommandList::setup(IRenderCommandList* commandList)
{
    if (commandList != cmdList)
    {
        cmdList = commandList;
    }
}

void RenderCommandList::newFrame()
{
    cmdList->newFrame();
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

void RenderCommandList::submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands)
{
    cmdList->submitCmds(priority, commands);
}

void RenderCommandList::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command)
{
    cmdList->submitCmd(priority, command);
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

void RenderCommandList::copyToImage(ImageResource* dst, const std::vector<class LinearColor>& pixelData, const CopyPixelsToImageInfo& copyInfo)
{
    cmdList->copyToImage(dst, pixelData, copyInfo);
}

void RenderCommandList::copyToImageLinearMapped(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo)
{
    cmdList->copyToImageLinearMapped(dst, pixelData, copyInfo);
}

void RenderCommandList::copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo)
{
    cmdList->copyOrResolveImage(src, dst, srcInfo, dstInfo);
}

void RenderCommandList::setupInitialLayout(ImageResource* image)
{
    cmdList->setupInitialLayout(image);
}

void RenderCommandList::presentImage(const std::vector<class GenericWindowCanvas*>& canvases, const std::vector<uint32>& imageIndices, const std::vector<SharedPtr<class GraphicsSemaphore>>& waitOnSemaphores)
{
    cmdList->presentImage(canvases, imageIndices, waitOnSemaphores);
}

void RenderCommandList::cmdBarrierResources(const GraphicsResource* cmdBuffer, const std::set<const ShaderParameters*>& descriptorsSets)
{
    cmdList->cmdBarrierResources(cmdBuffer, descriptorsSets);
}

void RenderCommandList::cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor)
{
    cmdList->cmdBeginRenderPass(cmdBuffer, contextPipeline, renderArea, renderpassAdditionalProps, clearColor);
}

void RenderCommandList::cmdEndRenderPass(const GraphicsResource* cmdBuffer)
{
    cmdList->cmdEndRenderPass(cmdBuffer);
}

void RenderCommandList::cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const
{
    cmdList->cmdBindGraphicsPipeline(cmdBuffer, contextPipeline, state);
}

void RenderCommandList::cmdBindComputePipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline) const
{
    cmdList->cmdBindComputePipeline(cmdBuffer, contextPipeline);
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

    // Mask from component's first byte
    uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
    for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
    {
        const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

        uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
        fatalAssert(((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx], "%s(): Component %d of pixel format %s is going beyond 32bits mask after offset"
            , __func__, compIdx, formatInfo->formatName.getChar());

        uint32 end = 1 << (formatInfo->componentSize[compIdx] - 1);
        uint32 mask = 1;
        perCompMask[compIdx] = 1;
        while ((mask & end) != end)
        {
            mask <<= 1;
            mask |= 1;
        }

        mask <<= offset;
        perCompMask[compIdx] = mask;
    }

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());

    if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
    {
        // Copying data
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8* pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
            {
                const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

                const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));
                const uint8 compSizeBits = formatInfo->componentSize[compIdx];

                // We are never going to go above 32 bits per channel
                const uint32 compValue = pixelData[i].getColorValue()[compIdx];

                uint8* offsetStagingPtr = pixelStagingPtr + (compOffset / colorCompBits);

                // Left shift
                const uint32 byte1Shift = compOffset % colorCompBits;

                (*reinterpret_cast<uint32*>(offsetStagingPtr)) |= (perCompMask[compIdx] & (compValue << byte1Shift));
            }
        }
    }
    else
    {
        fatalAssert(false, "Big endian platform not supported yet");
    }
}

void IRenderCommandList::copyPixelsTo(BufferResource* stagingBuffer, uint8* stagingPtr, const std::vector<class LinearColor>& pixelData, const EPixelDataFormat::PixelFormatInfo* formatInfo, bool bIsFloatingFormat) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    debugAssert(colorCompBits == 8);

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());
    if (bIsFloatingFormat)
    {
        for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
        {
            debugAssert(formatInfo->componentSize[uint32(formatInfo->componentOrder[idx])] == sizeof(float));
        }

        // Copying data
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8* pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
            {
                const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);
                const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));

                uint8* offsetStagingPtr = pixelStagingPtr + (compOffset / colorCompBits);

                memcpy(offsetStagingPtr, &pixelData[i].getColorValue()[compIdx], sizeof(float));
            }
        }
    }
    else
    {
        // Mask from component's first byte
        uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
        for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
        {
            const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

            uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
            fatalAssert(((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx], "%s(): Component %d of pixel format %s is going beyond 32bits mask after offset"
                , __func__, compIdx, formatInfo->formatName.getChar());

            uint32 end = 1 << (formatInfo->componentSize[compIdx] - 1);
            uint32 mask = 1;
            perCompMask[compIdx] = 1;
            while ((mask & end) != end)
            {
                mask <<= 1;
                mask |= 1;
            }

            mask <<= offset;
            perCompMask[compIdx] = mask;
        }

        if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
        {
            // Copying data
            for (uint32 i = 0; i < pixelData.size(); ++i)
            {
                uint8* pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
                for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
                {
                    const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

                    const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));
                    const uint8 compSizeBits = formatInfo->componentSize[compIdx];

                    // We are never going to go above 32 bits per channel
                    const uint32 maxVal = (Math::pow(2u, compSizeBits) - 1);
                    const uint32 compValue = uint32(pixelData[i].getColorValue()[compIdx] * maxVal);

                    uint8* offsetStagingPtr = pixelStagingPtr + (compOffset / colorCompBits);

                    // Left shift
                    const uint32 byte1Shift = compOffset % colorCompBits;

                    (*reinterpret_cast<uint32*>(offsetStagingPtr)) |= (perCompMask[compIdx] & (compValue << byte1Shift));
                }
            }
        }
        else
        {
            fatalAssert(false, "Big endian platform not supported yet");
        }
    }
}

void IRenderCommandList::copyPixelsLinearMappedTo(BufferResource* stagingBuffer, uint8* stagingPtr
    , const std::vector<class Color>& pixelData, const EPixelDataFormat::PixelFormatInfo* formatInfo) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    debugAssert(colorCompBits == 8);

    // Mask from component's first byte
    uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
    for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
    {
        const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

        uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
        fatalAssert(((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx], "%s(): Component %d of pixel format %s is going beyond 32bits mask after offset"
            , __func__, compIdx, formatInfo->formatName.getChar());

        uint32 end = 1 << (formatInfo->componentSize[compIdx] - 1);
        uint32 mask = 1;
        perCompMask[compIdx] = 1;
        while ((mask & end) != end)
        {
            mask <<= 1;
            mask |= 1;
        }

        mask <<= offset;
        perCompMask[compIdx] = mask;
    }

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());

    if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
    {
        // Copying data
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8* pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
            {
                const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

                const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));
                const uint8 compSizeBits = formatInfo->componentSize[compIdx];

                // We are never going to go above 32 bits per channel
                const uint32 maxVal = (Math::pow(2u, compSizeBits) - 1);
                const uint32 compValue = uint32((pixelData[i].getColorValue()[compIdx] / 255.0f) * maxVal);

                uint8* offsetStagingPtr = pixelStagingPtr + (compOffset / colorCompBits);

                // Left shift
                const uint32 byte1Shift = compOffset % colorCompBits;

                (*reinterpret_cast<uint32*>(offsetStagingPtr)) |= (perCompMask[compIdx] & (compValue << byte1Shift));
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
        Logger::error("RenderCommandList", "%s() : Texel data count is not sufficient to fill all texels of %s", __func__
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

void IRenderCommandList::copyToImageLinearMapped(ImageResource* dst, const std::vector<class Color>& pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        Logger::error("RenderCommandList", "%s() : Texel data count is not sufficient to fill all texels of %s", __func__
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
    copyToImageLinearMapped(dst, pixelData, copyInfo);
}

template <typename T>
struct PushConstCopier
{
    std::vector<uint8>& d;
    PushConstCopier(std::vector<uint8>& inData)
        :d(inData)
    {}

    void operator()(CopyBufferInfo& copyInfo, const std::any& data, const ReflectBufferEntry* field)
    {
        const T* value = std::any_cast<T>(&data);
        if (value)
        {
            copyInfo.copySize = sizeof(T);
            copyInfo.srcOffset = uint32(d.size());
            copyInfo.dstOffset = field->data.offset;

            d.resize(copyInfo.srcOffset + copyInfo.copySize);
            memcpy(&d[copyInfo.srcOffset], value, copyInfo.copySize);
        }
        else
        {
            Logger::error("RenderCommandList", "%s() : Cannot cast pushable constant %s", __func__, field->attributeName.c_str());
        }
    }
};

void IRenderCommandList::cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const std::vector<std::pair<String, std::any>>& pushData) const
{
    const ReflectPushConstant& entry = contextPipeline.getPipeline()->getShaderResource()->getReflection()->pushConstants;

    if (!entry.data.pushConstantField.bufferStructFields.empty())
    {
        Logger::warn("RenderCommandList", "%s() : [Shader: %s, Attribute: %s]Using SoS in push constant in not recommended"
            , __func__, contextPipeline.getPipeline()->getShaderResource()->getResourceName().getChar(), entry.attributeName.c_str());
    }

    if (entry.data.pushConstantField.bufferFields.empty() && entry.data.pushConstantField.bufferStructFields.empty())
    {
        return;
    }

    std::unordered_map<String, const ReflectBufferEntry*> nameToEntry;
    {
        std::vector<const ReflectBufferShaderField*> tree{ &entry.data.pushConstantField };

        for (uint32 i = 0; i < tree.size(); ++i)
        {
            const ReflectBufferShaderField* current = tree[i];
            for (const ReflectBufferEntry& field : current->bufferFields)
            {
                if (field.data.arraySize.size() != 1 || field.data.arraySize[0].isSpecializationConst || field.data.arraySize[0].dimension != 1)
                {
                    Logger::warn("RenderCommandList", "%s(): [Shader: %s, Attribute: %s] Array data is not supported in push constants"
                        , __func__, contextPipeline.getPipeline()->getShaderResource()->getResourceName().getChar(), field.attributeName.c_str());
                }
                else
                {
                    nameToEntry[field.attributeName] = &field;
                }
            }

            for (const ReflectBufferStructEntry& structField : current->bufferStructFields)
            {
                tree.emplace_back(&structField.data.data);
            }
        }
    }


    std::vector<uint8> data;
    std::vector<CopyBufferInfo> copies;
    for (const std::pair<String, std::any>& pushConst : pushData)
    {
        auto itr = nameToEntry.find(pushConst.first);
        if (itr == nameToEntry.end())
        {
            Logger::error("RenderCommandList", "%s() : Cannot find %s in pushable constants", __func__, pushConst.first.getChar());
            continue;
        }

        EShaderInputAttribFormat::Type format = EShaderInputAttribFormat::getInputFormat(itr->second->data.data.type);
        switch (format)
        {
        case EShaderInputAttribFormat::Float:
            PushConstCopier<float>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float2:
            PushConstCopier<Vector2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float3:
            PushConstCopier<Vector3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float4:
            PushConstCopier<Vector4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int:
            PushConstCopier<int32>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int2:
            PushConstCopier<Int2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int3:
            PushConstCopier<Int3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int4:
            PushConstCopier<Int4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt:
            PushConstCopier<uint32>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt2:
            PushConstCopier<Size2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt3:
            PushConstCopier<Size3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt4:
            PushConstCopier<Size4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte:
            PushConstCopier<uint8>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte2:
            PushConstCopier<Byte2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte3:
            PushConstCopier<Byte3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte4:
            PushConstCopier<Byte4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Matrix2x2:
            PushConstCopier<Matrix2>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Matrix3x3:
            PushConstCopier<Matrix3>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Matrix4x4:
            PushConstCopier<Matrix4>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Double:
        case EShaderInputAttribFormat::Double2:
        case EShaderInputAttribFormat::Double3:
        case EShaderInputAttribFormat::Double4:
        case EShaderInputAttribFormat::Byte:
        case EShaderInputAttribFormat::Byte2:
        case EShaderInputAttribFormat::Byte3:
        case EShaderInputAttribFormat::Byte4:
        case EShaderInputAttribFormat::ShortInt:
        case EShaderInputAttribFormat::ShortInt2:
        case EShaderInputAttribFormat::ShortInt3:
        case EShaderInputAttribFormat::ShortInt4:
        case EShaderInputAttribFormat::UShortInt:
        case EShaderInputAttribFormat::UShortInt2:
        case EShaderInputAttribFormat::UShortInt3:
        case EShaderInputAttribFormat::UShortInt4:
        case EShaderInputAttribFormat::UInt4Norm:
        case EShaderInputAttribFormat::Undefined:
        default:
            Logger::error("RenderCommandList", "%s(): [Shader: %s, Attribute: %s] Unsupported format %s in push constants"
                , __func__, contextPipeline.getPipeline()->getShaderResource()->getResourceName().getChar()
                , itr->second->attributeName.c_str(), pushConst.second.type().name());
            break;
        }
    }

    cmdPushConstants(cmdBuffer, contextPipeline, contextPipeline.getPipeline()->getShaderResource()->getReflection()->pushConstants.data.stagesUsed
        , data.data(), copies);
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