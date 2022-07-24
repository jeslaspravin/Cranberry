#pragma once

#include "EngineRendererExports.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"

class IRenderCommandList;
class IGraphicsInstance;
class GraphicsHelperAPI;

using RenderThreadEnqTask = copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread>;

class ENGINERENDERER_EXPORT RenderThreadEnqueuer
{
public:
    // Lambda is fine here
    using RenderEnqFuncType = LambdaFunction<void, IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *>;

    /**
     * Returns awaitable that can be awaited or waited on from another thread
     */
    static RenderThreadEnqTask execInRenderThreadAwaitable(RenderEnqFuncType execFunc);

    /**
     * Executes the passed in lambda in render thread and terminates. Fire and forget tasks can be enqueued this way
     */
    template <typename LambdaType>
    static void execInRenderingThread(LambdaType &&lambdaFunc)
    {
        // As purpose of enqueue is to execute in render thread not postpone execution if already in render thread.
        execInRenderingThreadOrImmediate(std::forward<LambdaType>(lambdaFunc));
    }

    static void flushWaitRenderThread();

private:
    static copat::NormalFuncAwaiter execInRenderingThreadOrImmediate(RenderEnqFuncType &&execFunc);
};

// CommandName is not used for now
#define ENQUEUE_RENDER_COMMAND(CommandName) RenderThreadEnqueuer::execInRenderingThread
#define ENQUEUE_COMMAND(CommandName) ENQUEUE_RENDER_COMMAND(CommandName)

#define ENQUEUE_RENDER_COMMAND_NODEBUG(CommandName, LambdaBody, ...)                                                                           \
    ENQUEUE_COMMAND(CommandName)                                                                                                               \
    ([##__VA_ARGS__##](IRenderCommandList * cmdList, IGraphicsInstance * graphicsInstance, const GraphicsHelperAPI *graphicsHelper)##LambdaBody)

#define ENQUEUE_COMMAND_NODEBUG(CommandName, LambdaBody, ...) ENQUEUE_RENDER_COMMAND_NODEBUG(CommandName, LambdaBody, __VA_ARGS__)

#define ASSERT_INSIDE_RENDERTHREAD()                                                                                                           \
    debugAssert(                                                                                                                               \
        copat::JobSystem::get() && copat::JobSystem::get()->getCurrentThreadType() == copat::EJobThreadType::RenderThread                      \
        && "Thread is not a render thread!"                                                                                                    \
    )