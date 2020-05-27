#include "IRenderCommandList.h"

class RenderCommandList final : public IRenderCommandList
{
private:
    IRenderCommandList* cmdList;

public:
    void setup(IRenderCommandList* commandList) override;

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) override;
    void copyToBuffer(const std::vector<BatchCopyData>& batchCopies) override;

    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) override;

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

void RenderCommandList::copyToBuffer(const std::vector<BatchCopyData>& batchCopies)
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

IRenderCommandList* IRenderCommandList::genericInstance()
{
    return new RenderCommandList();
}
