#pragma once

#include "../../Core/Platform/PlatformTypes.h"
#include "../../Core/Types/Functions.h"
#include "../GraphicsIntance.h"
#include "../../Core/Engine/GameEngine.h"

#include <type_traits>

class BufferResource;

class IRenderCommand
{
public:
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

class IRenderCommandList
{
public:
    virtual ~IRenderCommandList() = default;
    static IRenderCommandList* genericInstance();

    virtual void setup(IRenderCommandList* commandList) {};

    virtual void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) = 0;
    virtual void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) = 0;
};
