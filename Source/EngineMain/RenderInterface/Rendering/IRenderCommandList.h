#pragma once

#include "../../Core/Platform/PlatformTypes.h"
#include "../ShaderCore/ShaderParameters.h"
#include "../../Core/Types/Functions.h"
#include "../GraphicsIntance.h"
#include "../../Core/Engine/GameEngine.h"
#include "../Resources/QueueResource.h"

#include <type_traits>

class BufferResource;
class GraphicsResource;
class GraphicsFence;
struct CommandSubmitInfo;

class IRenderCommand
{
public:
    virtual ~IRenderCommand() = default;
    virtual void execute(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance) = 0;
};

// LambdaBody must be followed by all capture arguments
#define LAMBDA_BODY(...) { __VA_ARGS__ }

#define ENQUEUE_COMMAND(CommandName, LambdaBody, ...) \
class CommandName##_RenderCommand final : public IRenderCommand \
{ \
    using RenderCmdFunc = LambdaFunction<void, class IRenderCommandList*, IGraphicsInstance*>; \
    RenderCmdFunc renderCmd; \
public: \
    CommandName##_RenderCommand(RenderCmdFunc lambdaFunc) \
        : renderCmd(lambdaFunc) \
    {} \
    \
    void execute(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance) override \
    { \
        renderCmd(cmdList, graphicsInstance); \
    } \
}; \
gEngine->issueRenderCommand(new CommandName##_RenderCommand ({ [ ##__VA_ARGS__## ](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)##LambdaBody }))


struct CopyBufferInfo
{
    uint64 srcOffset;
    uint64 dstOffset;
    uint32 copySize;
};

struct BatchCopyData
{
    BufferResource* dst;
    uint32 dstOffset;
    const void* dataToCopy;
    uint32 size;
};

class IRenderCommandList
{
public:
    virtual ~IRenderCommandList() = default;
    static IRenderCommandList* genericInstance();

    virtual void setup(IRenderCommandList* commandList) {};

    virtual void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) = 0;
    virtual void copyToBuffer(const std::vector<BatchCopyData>& batchCopies) = 0;
    virtual void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) = 0;

    template<typename BufferDataType>
    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const BufferDataType* dataToCopy, const ShaderBufferParamInfo* bufferFields);

    ///////////////////////////////////////////////////////////////////////////////
    //// Command buffer related function access if you know what you are doing ////
    ///////////////////////////////////////////////////////////////////////////////

    // Reusable here mean rerecord able command buffer
    virtual const GraphicsResource* startCmd(String uniqueName, EQueueFunction queue, bool bIsReusable) = 0;
    virtual void endCmd(const GraphicsResource* cmdBuffer) = 0;
    // Frees the command buffer after usage
    virtual void freeCmd(const GraphicsResource* cmdBuffer) = 0;
    virtual void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo
        , const SharedPtr<GraphicsFence>& fence) = 0;
    virtual void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo) = 0;

    // Waits until gpu is idle
    virtual void waitIdle() = 0;
};

template<typename BufferDataType>
void IRenderCommandList::copyToBuffer(BufferResource* dst, uint32 dstOffset, const BufferDataType* dataToCopy
    , const ShaderBufferParamInfo* bufferFields)
{
    std::vector<BatchCopyData> batchedCopies;

    const ShaderBufferFieldNode* fieldNode = &bufferFields->startNode;
    while (fieldNode->isValid())
    {
        auto* bufferMemberField = static_cast<ShaderBufferTypedField<BufferDataType>*>(fieldNode->field);

        BatchCopyData copyData;
        copyData.dst = dst;
        copyData.dstOffset = dstOffset + bufferMemberField->offset;
        copyData.dataToCopy = bufferMemberField->fieldData(copyData.size, dataToCopy);
        batchedCopies.push_back(copyData);

        fieldNode = fieldNode->nextNode;
    }
    copyToBuffer(batchedCopies);
}
