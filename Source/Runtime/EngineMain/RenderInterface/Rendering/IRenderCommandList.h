#pragma once

#include <optional>
#include <set>

#include "../../Core/Platform/PlatformTypes.h"
#include "../ShaderCore/ShaderParameters.h"
#include "../../Core/Types/Functions.h"
#include "../GraphicsIntance.h"
#include "../../Core/Engine/GameEngine.h"
#include "../Resources/QueueResource.h"
#include "../Resources/MemoryResources.h"
#include "../../Core/Math/Box.h"
#include "../../Core/Types/Colors.h"
#include "../Resources/Pipelines.h"

class BufferResource;
class GraphicsResource;
class GraphicsFence;
struct CommandSubmitInfo;
struct CommandSubmitInfo2;
class ImageResource;
class LocalPipelineContext;
struct RenderPassAdditionalProps;
class ShaderParameters;

class IRenderCommand
{
public:
    virtual ~IRenderCommand() = default;
    virtual void execute(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance) = 0;
};


template<typename CommandType, typename LambdaType>
void issueRenderCommand(LambdaType&& lambdaFunc)
{
    GameEngine::issueRenderCommand<CommandType>(typename CommandType::RenderCmdFunc(std::forward<LambdaType>(lambdaFunc)));
};

// LambdaBody must be followed by all capture arguments
#define LAMBDA_BODY(...) { __VA_ARGS__ }

#define ENQUEUE_COMMAND(CommandName) \
class CommandName##_RenderCommand final : public IRenderCommand \
{ \
public: \
    using RenderCmdFunc = LambdaFunction<void, class IRenderCommandList*, IGraphicsInstance*>; \
private: \
    RenderCmdFunc renderCmd; \
public: \
    CommandName##_RenderCommand(RenderCmdFunc &&lambdaFunc) \
        : renderCmd(std::forward<RenderCmdFunc>(lambdaFunc)) \
    {} \
    \
    void execute(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance) override \
    { \
        renderCmd(cmdList, graphicsInstance); \
    } \
}; \
::issueRenderCommand<CommandName##_RenderCommand>


#define ENQUEUE_COMMAND_NODEBUG(CommandName, LambdaBody, ...) \
    ENQUEUE_COMMAND(CommandName) \
    ([ ##__VA_ARGS__## ](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)##LambdaBody )

#define SCOPED_CMD_MARKER(CmdList,CommandBuffer,Name) ScopedCommandMarker cmdMarker_##Name(CmdList, CommandBuffer, #Name)
#define SCOPED_CMD_COLORMARKER(CmdList,CommandBuffer,Name,Color) ScopedCommandMarker cmdMarker_##Name(CmdList, CommandBuffer, #Name, Color)
struct ScopedCommandMarker
{
    const GraphicsResource* cmdBuffer;
    const class IRenderCommandList* cmdList;
    ScopedCommandMarker(const class IRenderCommandList* commandList, const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE);
    ~ScopedCommandMarker();
};


struct CopyBufferInfo
{
    uint64 srcOffset;
    uint64 dstOffset;
    uint32 copySize;
};
// Single copy from src to dst
struct BatchCopyBufferInfo
{
    BufferResource* src;
    BufferResource* dst; 
    CopyBufferInfo copyInfo;
};

// Single copy struct, Used to copy cpu visible data to a gpu(can be cpu visible) buffer
struct BatchCopyBufferData
{
    BufferResource* dst;
    uint32 dstOffset;
    const void* dataToCopy;
    uint32 size;
};

struct CopyPixelsToImageInfo
{
    // Offset and extent for MIP base rest will be calculated automatically
    Size3D srcOffset;
    Size3D dstOffset;
    Size3D extent;

    ImageSubresource subres;

    bool bGenerateMips;
    // Filtering to be used to generate MIPs
    ESamplerFiltering::Type mipFiltering;
};

struct CopyImageInfo
{
    // Offset and extent for MIP base rest will be calculated automatically
    Size3D offset{ 0 };
    Size3D extent;

    ImageSubresource subres;

    FORCE_INLINE bool isCopyCompatible(const CopyImageInfo& rhs) const
    {
        return extent == rhs.extent && subres == rhs.subres;
    }
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

class IRenderCommandList
{
protected:
    // Copies to known format from color, returns true if copied
    bool simpleCopyPixelsTo(BufferResource* stagingBuffer, uint8* stagingPtr, const std::vector<class Color>& pixelData, EPixelDataFormat::Type dataFormat, const EPixelDataFormat::PixelFormatInfo* formatInfo) const;

    // raw copies the pixels to staging buffer, Only accepts non floating point or 32bit floating point textures
    void copyPixelsTo(BufferResource* stagingBuffer, uint8* stagingPtr, const std::vector<class Color>& pixelData, const EPixelDataFormat::PixelFormatInfo* formatInfo) const;
    void copyPixelsLinearMappedTo(BufferResource* stagingBuffer, uint8* stagingPtr, const std::vector<class Color>& pixelData, const EPixelDataFormat::PixelFormatInfo* formatInfo) const;
    void copyPixelsTo(BufferResource* stagingBuffer, uint8* stagingPtr, const std::vector<class LinearColor>& pixelData, const EPixelDataFormat::PixelFormatInfo* formatInfo, bool bIsFloatingFormat) const;
public:
    virtual ~IRenderCommandList() = default;
    static IRenderCommandList* genericInstance();

    virtual void setup(IRenderCommandList* commandList) {};
    virtual void newFrame(const float& tiimeDelta) = 0;

    virtual void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) = 0;
    virtual void copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies) = 0;
    virtual void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) = 0;
    virtual void copyBuffer(const std::vector<BatchCopyBufferInfo>& batchCopies) = 0;
    // Below copies does not take inner structure alignment and offset into account so do not use this to copy structures that has inner structure which is not tightly packed
    template<typename BufferDataType>
    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const BufferDataType* dataToCopy, const ShaderBufferParamInfo* bufferFields);
    template<typename BufferDataType>
    void recordCopyToBuffer(std::vector<BatchCopyBufferData>& recordTo, BufferResource* dst, uint32 dstOffset, const BufferDataType* dataToCopy, const ShaderBufferParamInfo* bufferFields);

    // Copy pixel data to only first MIP level of all layers and generate the rest of MIP
    void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData);
    void copyToImageLinearMapped(ImageResource* dst, const std::vector<class Color>& pixelData);

    virtual void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) = 0;
    // Linear maps each pixel to component byte range
    virtual void copyToImageLinearMapped(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) = 0;
    virtual void copyToImage(ImageResource* dst, const std::vector<class LinearColor>& pixelData, const CopyPixelsToImageInfo& copyInfo) = 0;
    virtual void copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) = 0;

    virtual void clearImage(ImageResource* image, const LinearColor& clearColor, const std::vector<ImageSubresource>& subresources) = 0;
    virtual void clearDepth(ImageResource* image, float depth, uint32 stencil, const std::vector<ImageSubresource>& subresources) = 0;

    virtual void setupInitialLayout(ImageResource* image) = 0;

    virtual void presentImage(const std::vector<class GenericWindowCanvas*>& canvases,
        const std::vector<uint32>& imageIndices, const std::vector<SharedPtr<class GraphicsSemaphore>>& waitOnSemaphores) = 0;

    ///////////////////////////////////////////////////////////////////////////////
    //// Command buffer related function access if you know what you are doing ////
    ///////////////////////////////////////////////////////////////////////////////

    virtual void cmdCopyOrResolveImage(const GraphicsResource* cmdBuffer, ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) = 0;
    // Transitions the layout of image to general identified usage, for color/depth attachments sample read will be after transition layout
    virtual void cmdTransitionLayouts(const GraphicsResource* cmdBuffer, const std::vector<ImageResource*>& images) = 0;

    virtual void cmdClearImage(const GraphicsResource* cmdBuffer, ImageResource* image, const LinearColor& clearColor, const std::vector<ImageSubresource>& subresources) = 0;
    virtual void cmdClearDepth(const GraphicsResource* cmdBuffer, ImageResource* image, float depth, uint32 stencil, const std::vector<ImageSubresource>& subresources) = 0;

    virtual void cmdBarrierResources(const GraphicsResource* cmdBuffer, const std::set<const ShaderParameters*>& descriptorsSets) = 0;

    virtual void cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor) = 0;
    virtual void cmdEndRenderPass(const GraphicsResource* cmdBuffer) = 0;

    virtual void cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const = 0;
    virtual void cmdBindComputePipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline) const = 0;
    void cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const std::vector<std::pair<String, std::any>>& pushData) const;
    virtual void cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, uint32 stagesUsed, const uint8* data, const std::vector<CopyBufferInfo>& pushConsts) const {};
    void cmdBindDescriptorsSets(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const ShaderParameters* descriptorsSets) const;
    void cmdBindDescriptorsSets(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const;
    virtual void cmdBindDescriptorsSetInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::map<uint32, const ShaderParameters*>& descriptorsSets) const = 0;
    virtual void cmdBindDescriptorsSetsInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const = 0;
    // Offset in bytes
    virtual void cmdBindVertexBuffers(const GraphicsResource* cmdBuffer, uint32 firstBinding, const std::vector<const BufferResource*>& vertexBuffers, const std::vector<uint64>& offsets) const = 0;
    virtual void cmdBindIndexBuffer(const GraphicsResource* cmdBuffer, const BufferResource* indexBuffer, uint64 offset = 0) const = 0;

    virtual void cmdDispatch(const GraphicsResource* cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ = 1) const = 0;
    // vertexOffset is vertex to add before indexing that vertex in vertex buffer 
    virtual void cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance = 0, uint32 instanceCount = 1, int32 vertexOffset = 0) const = 0;
    virtual void cmdDrawVertices(const GraphicsResource* cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance = 0, uint32 instanceCount = 1) const = 0;
    // bufferOffset - Offset in bytes, stride - useful when draw struct is part of some complex AoS
    virtual void cmdDrawIndexedIndirect(const GraphicsResource* cmdBuffer, const BufferResource* drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride) const = 0;
    virtual void cmdDrawIndirect(const GraphicsResource* cmdBuffer, const BufferResource* drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride) const = 0;

    virtual void cmdSetViewportAndScissors(const GraphicsResource* cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>>& viewportAndScissors, uint32 firstViewport = 0) const = 0;
    virtual void cmdSetViewportAndScissor(const GraphicsResource* cmdBuffer, const QuantizedBox2D& viewport, const QuantizedBox2D& scissor, uint32 atViewport = 0) const = 0;
    virtual void cmdSetLineWidth(const GraphicsResource* cmdBuffer, float lineWidth) const = 0;
    virtual void cmdSetDepthBias(const GraphicsResource* cmdBuffer, float constantBias, float slopeFactor, float clampValue) const = 0;

    virtual void cmdBeginBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const = 0;
    virtual void cmdInsertBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const = 0;
    virtual void cmdEndBufferMarker(const GraphicsResource* commandBuffer) const = 0;

    // Reusable here mean rerecord able command buffer
    virtual const GraphicsResource* startCmd(const String& uniqueName, EQueueFunction queue, bool bIsReusable) = 0;
    virtual void endCmd(const GraphicsResource* cmdBuffer) = 0;
    // Frees the command buffer after usage
    virtual void freeCmd(const GraphicsResource* cmdBuffer) = 0;
    virtual void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo
        , const SharedPtr<GraphicsFence>& fence) = 0;

    virtual void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& submitInfo) = 0;
    virtual void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands) = 0;
    virtual void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command) = 0;

    virtual void finishCmd(const GraphicsResource* cmdBuffer) = 0;
    virtual void finishCmd(const String& uniqueName) = 0;
    virtual const GraphicsResource* getCmdBuffer(const String& uniqueName) const = 0;

    // Waits until GPU is idle
    virtual void waitIdle() = 0;
    virtual void flushAllcommands() = 0;
};

template<typename BufferDataType>
void IRenderCommandList::copyToBuffer(BufferResource* dst, uint32 dstOffset, const BufferDataType* dataToCopy
    , const ShaderBufferParamInfo* bufferFields)
{
    std::vector<BatchCopyBufferData> batchedCopies;
    for (const ShaderBufferField* bufferField : *bufferFields)
    {
        auto* bufferMemberField = static_cast<const ShaderBufferTypedField<BufferDataType>*>(bufferField);

        BatchCopyBufferData copyData;
        copyData.dst = dst;
        copyData.dstOffset = dstOffset + bufferMemberField->offset;
        copyData.dataToCopy = bufferMemberField->fieldData(dataToCopy, &copyData.size, nullptr);
        batchedCopies.push_back(copyData);
    }
    copyToBuffer(batchedCopies);
}

template<typename BufferDataType>
void IRenderCommandList::recordCopyToBuffer(std::vector<BatchCopyBufferData>& recordTo, BufferResource* dst
    , uint32 dstOffset, const BufferDataType* dataToCopy, const ShaderBufferParamInfo* bufferFields)
{
    for (const ShaderBufferField* bufferField : *bufferFields)
    {
        auto* bufferMemberField = static_cast<const ShaderBufferTypedField<BufferDataType>*>(bufferField);

        BatchCopyBufferData copyData;
        copyData.dst = dst;
        copyData.dstOffset = dstOffset + bufferMemberField->offset;
        copyData.dataToCopy = bufferMemberField->fieldData(dataToCopy, &copyData.size, nullptr);
        recordTo.push_back(copyData);
    }
}