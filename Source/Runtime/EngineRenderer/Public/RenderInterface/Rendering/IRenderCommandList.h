/*!
 * \file IRenderCommandList.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <optional>
#include <set>
#include <unordered_map>

#include "Math/Box.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/Resources/QueueResource.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "Types/Colors.h"

struct CommandSubmitInfo;
struct CommandSubmitInfo2;
class LocalPipelineContext;
struct RenderPassAdditionalProps;
struct RenderPassClearValue;
class Color;
class LinearColor;
class IRenderCommandList;

#define SCOPED_CMD_MARKER(CmdList, CommandBuffer, Name)                                                                                        \
    ScopedCommandMarker cmdMarker_##Name(CmdList, CommandBuffer, TCHAR(#Name));                                                                \
    CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR(#Name))

#define SCOPED_STR_CMD_MARKER(CmdList, CommandBuffer, Name)                                                                                    \
    ScopedCommandMarker COMBINE(cmdMarker_, __COUNTER__)(CmdList, CommandBuffer, (Name));                                                      \
    CBE_PROFILER_TSCOPE(Name.getChar())

#define SCOPED_CMD_COLORMARKER(CmdList, CommandBuffer, Name, Color)                                                                            \
    ScopedCommandMarker cmdMarker_##Name(CmdList, CommandBuffer, TCHAR(#Name), Color);                                                         \
    CBE_PROFILER_SCOPE_C(CBE_PROFILER_CHAR(#Name), Color)

struct ScopedCommandMarker
{
    const GraphicsResource *cmdBuffer;
    const IRenderCommandList *cmdList;
    FORCE_INLINE ScopedCommandMarker(
        const IRenderCommandList *commandList, const GraphicsResource *commandBuffer, const String &name,
        const LinearColor &color = LinearColorConst::WHITE
    );
    FORCE_INLINE ~ScopedCommandMarker();
};

#define SCOPED_RENDERPASS(CmdList, CommandBuffer, PipelineContext, RenderArea, AdditionalProps, ClearVal, RenderPassName)                      \
    ScopedRenderPass renderPass_##RenderPassName(CmdList, CommandBuffer, PipelineContext, RenderArea, AdditionalProps, ClearVal)
struct ScopedRenderPass
{
    const GraphicsResource *cmdBuffer;
    IRenderCommandList *cmdList;

    FORCE_INLINE ScopedRenderPass(
        IRenderCommandList *commandList, const GraphicsResource *commandBuffer, const LocalPipelineContext &contextPipeline,
        const IRect &renderArea, const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
    );

    FORCE_INLINE ~ScopedRenderPass();
};

struct RenderPassClearValue
{
    std::vector<LinearColor> colors;
    float depth = 0;
    uint32 stencil = 0;
};

struct GraphicsPipelineState
{
    GraphicsPipelineQueryParams pipelineQuery;
    // Dynamic states
    std::optional<LinearColor> blendConstant;
    std::vector<std::pair<EStencilFaceMode, uint32>> stencilReferences;
    std::optional<float> lineWidth;
};

struct DrawIndirectCommand
{
    uint32 vertexCount;
    uint32 instanceCount;
    uint32 firstVertex;
    uint32 firstInstance;
};

struct DrawIndexedIndirectCommand
{
    uint32 indexCount;
    uint32 instanceCount;
    uint32 firstIndex;
    // vertexOffset is vertex to add before indexing that vertex in vertex buffer
    int32 vertexOffset;
    uint32 firstInstance;
};

class ENGINERENDERER_EXPORT IRenderCommandList
{
protected:
    // Copies to known format from color, returns true if copied
    bool simpleCopyPixelsTo(
        BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<Color> pixelData, EPixelDataFormat::Type dataFormat,
        const EPixelDataFormat::PixelFormatInfo *formatInfo
    ) const;

    // raw copies the pixels to staging buffer, Only accepts non floating point or 32bit floating point
    // textures
    void copyPixelsTo(
        BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<Color> pixelData, const EPixelDataFormat::PixelFormatInfo *formatInfo
    ) const;
    void copyPixelsLinearMappedTo(
        BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<Color> pixelData, const EPixelDataFormat::PixelFormatInfo *formatInfo
    ) const;
    void copyPixelsTo(
        BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<LinearColor> pixelData,
        const EPixelDataFormat::PixelFormatInfo *formatInfo, bool bIsFloatingFormat
    ) const;

public:
    virtual ~IRenderCommandList() = default;
    static IRenderCommandList *genericInstance();

    virtual void setup(IRenderCommandList *){};
    virtual void newFrame(float timeDelta) = 0;

    virtual void copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size) = 0;
    virtual void copyToBuffer(ArrayView<BatchCopyBufferData> batchCopies) = 0;
    virtual void copyBuffer(BufferResourceRef src, BufferResourceRef dst, ArrayView<CopyBufferInfo> copies) = 0;
    virtual void copyBuffer(ArrayView<BatchCopyBufferInfo> batchCopies) = 0;
    // Below copies does not take inner structure alignment and offset into account so do not use this to
    // copy structures that has inner structure which is not tightly packed
    template <typename BufferDataType>
    void copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const BufferDataType *dataToCopy, const ShaderBufferParamInfo *bufferFields);
    template <typename BufferDataType>
    void recordCopyToBuffer(
        std::vector<BatchCopyBufferData> &recordTo, BufferResourceRef dst, uint32 dstOffset, const BufferDataType *dataToCopy,
        const ShaderBufferParamInfo *bufferFields
    );
    void recordCopyToBuffer(
        std::vector<BatchCopyBufferData> &recordTo, BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy,
        const ShaderBufferParamInfo *bufferFields
    );

    // Copy pixel data to only first MIP level of all layers and generate the rest of MIP
    void copyToImage(ImageResourceRef dst, ArrayView<Color> pixelData);
    void copyToImageLinearMapped(ImageResourceRef dst, ArrayView<Color> pixelData);

    virtual void copyToImage(ImageResourceRef dst, ArrayView<Color> pixelData, const CopyPixelsToImageInfo &copyInfo) = 0;
    // Linear maps each pixel to component byte range
    virtual void copyToImageLinearMapped(ImageResourceRef dst, ArrayView<Color> pixelData, const CopyPixelsToImageInfo &copyInfo) = 0;
    virtual void copyToImage(ImageResourceRef dst, ArrayView<LinearColor> pixelData, const CopyPixelsToImageInfo &copyInfo) = 0;
    virtual void copyOrResolveImage(ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo) = 0;

    virtual void clearImage(ImageResourceRef image, const LinearColor &clearColor, ArrayView<ImageSubresource> subresources) = 0;
    virtual void clearDepth(ImageResourceRef image, float depth, uint32 stencil, ArrayView<ImageSubresource> subresources) = 0;

    virtual void setupInitialLayout(ImageResourceRef image) = 0;

    virtual void presentImage(ArrayView<WindowCanvasRef> canvases, ArrayView<uint32> imageIndices, ArrayView<SemaphoreRef> waitOnSemaphores)
        = 0;

    ///////////////////////////////////////////////////////////////////////////////
    //// Command buffer related function access if you know what you are doing ////
    ///////////////////////////////////////////////////////////////////////////////

    virtual void
    cmdCopyBuffer(const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst, ArrayView<CopyBufferInfo> copies)
        = 0;
    virtual void cmdCopyBuffer(const GraphicsResource *cmdBuffer, ArrayView<BatchCopyBufferInfo> copies) = 0;
    virtual void cmdCopyToBuffer(const GraphicsResource *cmdBuffer, ArrayView<BatchCopyBufferData> batchCopies) = 0;
    virtual void cmdCopyOrResolveImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo,
        const CopyImageInfo &dstInfo
    ) = 0;
    // Transitions the layout of image to general identified usage, for color/depth attachments sample
    // read will be after transition layout
    virtual void cmdTransitionLayouts(const GraphicsResource *cmdBuffer, ArrayView<ImageResourceRef> images) = 0;

    virtual void cmdClearImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, const LinearColor &clearColor, ArrayView<ImageSubresource> subresources
    ) = 0;
    virtual void cmdClearDepth(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, float depth, uint32 stencil, ArrayView<ImageSubresource> subresources
    ) = 0;

    // descriptor sets must be unique
    virtual void cmdBarrierResources(const GraphicsResource *cmdBuffer, ArrayView<ShaderParametersRef> descriptorsSets) = 0;
    // Below are so that barriers will be able to be applied outside render pass
    virtual void cmdBarrierVertices(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> vertexBuffers) = 0;
    virtual void cmdBarrierIndices(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> indexBuffers) = 0;
    virtual void cmdBarrierIndirectDraws(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> indirectDrawBuffers) = 0;
    // For queue transfers
    virtual void cmdReleaseQueueResources(const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue) = 0;
    virtual void cmdReleaseQueueResources(
        const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue,
        const std::unordered_map<MemoryResourceRef, EQueueFunction> &perResourceRelease
    ) = 0;

    virtual void cmdBeginRenderPass(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const IRect &renderArea,
        const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
    ) = 0;
    virtual void cmdEndRenderPass(const GraphicsResource *cmdBuffer) = 0;

    virtual void cmdBindGraphicsPipeline(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const GraphicsPipelineState &state
    ) const
        = 0;
    virtual void cmdBindComputePipeline(const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline) const = 0;
    void cmdPushConstants(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, ArrayView<std::pair<String, std::any>> pushData
    ) const;

    virtual void cmdPushConstants(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, uint32 stagesUsed, const uint8 *data,
        ArrayView<CopyBufferInfo> pushConsts
    ) const
        = 0;
    void cmdBindDescriptorsSets(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, ShaderParametersRef descriptorsSets
    ) const;
    void cmdBindDescriptorsSets(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, ArrayView<ShaderParametersRef> descriptorsSets
    ) const;
    virtual void cmdBindDescriptorsSetInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::map<uint32, ShaderParametersRef> &descriptorsSets
    ) const
        = 0;
    virtual void cmdBindDescriptorsSetsInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, ArrayView<ShaderParametersRef> descriptorsSets
    ) const
        = 0;
    // Offset in bytes
    virtual void cmdBindVertexBuffer(const GraphicsResource *cmdBuffer, uint32 firstBinding, BufferResourceRef vertexBuffer, uint64 offset) = 0;
    virtual void cmdBindVertexBuffers(
        const GraphicsResource *cmdBuffer, uint32 firstBinding, ArrayView<BufferResourceRef> vertexBuffers, ArrayView<uint64> offsets
    ) = 0;
    virtual void cmdBindIndexBuffer(const GraphicsResource *cmdBuffer, const BufferResourceRef &indexBuffer, uint64 offset = 0) = 0;

    virtual void cmdDispatch(const GraphicsResource *cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ = 1) const = 0;
    // vertexOffset is vertex to add before indexing that vertex in vertex buffer
    virtual void cmdDrawIndexed(
        const GraphicsResource *cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance = 0, uint32 instanceCount = 1,
        int32 vertexOffset = 0
    ) const
        = 0;
    virtual void cmdDrawVertices(
        const GraphicsResource *cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance = 0, uint32 instanceCount = 1
    ) const
        = 0;
    // bufferOffset - Offset in bytes, stride - useful when draw struct is part of some complex AoS
    virtual void cmdDrawIndexedIndirect(
        const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
    ) = 0;
    virtual void cmdDrawIndirect(
        const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
    ) = 0;

    virtual void cmdSetViewportAndScissors(
        const GraphicsResource *cmdBuffer, ArrayView<std::pair<IRect, IRect>> viewportAndScissors, uint32 firstViewport = 0
    ) const
        = 0;
    virtual void
    cmdSetViewportAndScissor(const GraphicsResource *cmdBuffer, const IRect &viewport, const IRect &scissor, uint32 atViewport = 0) const
        = 0;
    // Usually you do one viewport and scissor set and several scissor set after that, So having separate scissor set cmd
    virtual void cmdSetScissor(const GraphicsResource *cmdBuffer, const IRect &scissor, uint32 atViewport = 0) const = 0;
    virtual void cmdSetLineWidth(const GraphicsResource *cmdBuffer, float lineWidth) const = 0;
    virtual void cmdSetDepthBias(const GraphicsResource *cmdBuffer, float constantBias, float slopeFactor, float clampValue) const = 0;

    virtual void
    cmdBeginBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color = LinearColorConst::WHITE) const
        = 0;
    virtual void
    cmdInsertBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color = LinearColorConst::WHITE) const
        = 0;
    virtual void cmdEndBufferMarker(const GraphicsResource *commandBuffer) const = 0;

    // Reusable here mean rerecord able command buffer
    virtual const GraphicsResource *startCmd(const String &uniqueName, EQueueFunction queue, bool bIsReusable) = 0;
    virtual void endCmd(const GraphicsResource *cmdBuffer) = 0;
    // Frees the command buffer after usage
    virtual void freeCmd(const GraphicsResource *cmdBuffer) = 0;

    /**
     * Advanced submits, Dependencies needs to be handled manually. If you are using cmdBarrierResources() in this command buffer use
     * CommandSubmitInfo2 alternative
     */
    virtual void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo &submitInfo, FenceRef fence) = 0;
    virtual void submitCmds(EQueuePriority::Enum priority, ArrayView<CommandSubmitInfo> submitInfos, FenceRef fence) = 0;

    virtual void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &submitInfo) = 0;
    virtual void submitCmds(EQueuePriority::Enum priority, ArrayView<CommandSubmitInfo2> submitInfos) = 0;
    virtual void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &command) = 0;

    virtual void finishCmd(const GraphicsResource *cmdBuffer) = 0;
    virtual void finishCmd(const String &uniqueName) = 0;

    virtual const GraphicsResource *getCmdBuffer(const String &uniqueName) const = 0;
    virtual TimelineSemaphoreRef getCmdSignalSemaphore(const String &uniqueName) const = 0;
    virtual TimelineSemaphoreRef getCmdSignalSemaphore(const GraphicsResource *cmdBuffer) const = 0;

    // Waits until GPU is idle
    virtual void waitIdle() = 0;
    // Waits until all cmds that uses this resource is terminated, However the resource must be manually
    // marked by using cmdBarrierResources() function
    virtual void waitOnResDepCmds(const MemoryResourceRef &resource) = 0;
    virtual void flushAllcommands() = 0;

    virtual bool hasCmdsUsingResource(const MemoryResourceRef &resource, bool bFinishCmds) = 0;
};

template <typename BufferDataType>
void IRenderCommandList::copyToBuffer(
    BufferResourceRef dst, uint32 dstOffset, const BufferDataType *dataToCopy, const ShaderBufferParamInfo *bufferFields
)
{
    std::vector<BatchCopyBufferData> batchedCopies;
    for (const ShaderBufferField *bufferField : *bufferFields)
    {
        auto *bufferMemberField = static_cast<const ShaderBufferTypedField<BufferDataType> *>(bufferField);

        BatchCopyBufferData copyData;
        copyData.dst = dst;
        copyData.dstOffset = dstOffset + bufferMemberField->offset;
        copyData.dataToCopy = bufferMemberField->fieldData(dataToCopy, &copyData.size, nullptr);
        batchedCopies.push_back(copyData);
    }
    copyToBuffer(batchedCopies);
}

template <typename BufferDataType>
void IRenderCommandList::recordCopyToBuffer(
    std::vector<BatchCopyBufferData> &recordTo, BufferResourceRef dst, uint32 dstOffset, const BufferDataType *dataToCopy,
    const ShaderBufferParamInfo *bufferFields
)
{
    for (const ShaderBufferField *bufferField : *bufferFields)
    {
        auto *bufferMemberField = static_cast<const ShaderBufferTypedField<BufferDataType> *>(bufferField);

        BatchCopyBufferData copyData;
        copyData.dst = dst;
        copyData.dstOffset = dstOffset + bufferMemberField->offset;
        copyData.dataToCopy = bufferMemberField->fieldData(dataToCopy, &copyData.size, nullptr);
        recordTo.push_back(copyData);
    }
}

template <typename BufferType>
bool ShaderParameters::setBuffer(StringID paramName, const BufferType &bufferValue, uint32 index /* = 0 */)
{
    bool bValueSet = false;
    auto bufferDataItr = shaderBuffers.find(paramName);

    if (bufferDataItr == shaderBuffers.end())
    {
        StringID bufferName;
        std::pair<const BufferParametersData *, const BufferParametersData::BufferParameter *> foundInfo
            = findBufferParam(bufferName, paramName);
        if (foundInfo.first && foundInfo.second && BIT_SET(foundInfo.second->bufferField->fieldDecorations, ShaderBufferField::IsStruct)
            && (!foundInfo.second->bufferField->isPointer() || foundInfo.first->runtimeArray->currentCount > index))
        {
            if (foundInfo.second->bufferField->isIndexAccessible())
            {
                if (bValueSet = foundInfo.second->bufferField->setFieldDataArray(foundInfo.second->outerPtr, bufferValue, index))
                {
                    genericUpdates.emplace_back(
                        [foundInfo, index](ParamUpdateLambdaOut &paramOut, IRenderCommandList *cmdList, IGraphicsInstance *)
                        {
                            BufferType *bufferPtr = reinterpret_cast<BufferType *>(
                                foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, nullptr)
                            );
                            cmdList->recordCopyToBuffer<BufferType>(
                                *paramOut.bufferUpdates, foundInfo.first->gpuBuffer,
                                foundInfo.second->bufferField->offset + (index * foundInfo.second->bufferField->stride), bufferPtr + index,
                                foundInfo.second->bufferField->paramInfo
                            );
                        }
                    );
                }
            }
            else if (bValueSet = foundInfo.second->bufferField->setFieldData(foundInfo.second->outerPtr, bufferValue))
            {
                genericUpdates.emplace_back(
                    [foundInfo](ParamUpdateLambdaOut &paramOut, IRenderCommandList *cmdList, IGraphicsInstance *)
                    {
                        cmdList->recordCopyToBuffer<BufferType>(
                            *paramOut.bufferUpdates, foundInfo.first->gpuBuffer, foundInfo.second->bufferField->offset,
                            reinterpret_cast<BufferType *>(
                                foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, nullptr)
                            ),
                            foundInfo.second->bufferField->paramInfo
                        );
                    }
                );
            }
        }
        else
        {
            LOG_ERROR("ShaderParameters", "Cannot set {}[{}] of {}", paramName, index, bufferName);
        }
    }
    else
    {
        BufferParametersData *bufferDataPtr = &bufferDataItr->second;
        bValueSet = (bufferDataPtr->descriptorInfo->bufferParamInfo->paramNativeStride() == sizeof(BufferType))
                    && !bufferDataPtr->runtimeArray.has_value();
        if (bValueSet)
        {
            (*reinterpret_cast<BufferType *>(bufferDataPtr->cpuBuffer)) = bufferValue;
            genericUpdates.emplace_back(
                [bufferDataPtr](ParamUpdateLambdaOut &paramOut, IRenderCommandList *cmdList, IGraphicsInstance *)
                {
                    cmdList->recordCopyToBuffer<BufferType>(
                        *paramOut.bufferUpdates, bufferDataPtr->gpuBuffer, 0, reinterpret_cast<BufferType *>(bufferDataPtr->cpuBuffer),
                        bufferDataPtr->descriptorInfo->bufferParamInfo
                    );
                }
            );
        }
        else
        {
            LOG_ERROR(
                "ShaderParameters",
                "Cannot set stride {} to stride {} or cannot set buffer with runtime "
                "array as single struct, Set runtime array separately",
                sizeof(BufferType), bufferDataPtr->descriptorInfo->bufferParamInfo->paramNativeStride()
            );
        }
    }
    return bValueSet;
}

FORCE_INLINE ScopedCommandMarker::ScopedCommandMarker(
    const IRenderCommandList *commandList, const GraphicsResource *commandBuffer, const String &name,
    const LinearColor &color /*= LinearColorConst::WHITE*/
)
    : cmdList(commandList)
    , cmdBuffer(commandBuffer)
{
    cmdList->cmdBeginBufferMarker(cmdBuffer, name, color);
}

FORCE_INLINE ScopedCommandMarker::~ScopedCommandMarker() { cmdList->cmdEndBufferMarker(cmdBuffer); }

FORCE_INLINE ScopedRenderPass::ScopedRenderPass(
    IRenderCommandList *commandList, const GraphicsResource *commandBuffer, const LocalPipelineContext &contextPipeline,
    const IRect &renderArea, const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
)
    : cmdList(commandList)
    , cmdBuffer(commandBuffer)
{
    cmdList->cmdBeginRenderPass(cmdBuffer, contextPipeline, renderArea, renderpassAdditionalProps, clearColor);
}

FORCE_INLINE ScopedRenderPass::~ScopedRenderPass() { cmdList->cmdEndRenderPass(cmdBuffer); }
