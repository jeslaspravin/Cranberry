#pragma once

#include "EngineRendererExports.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"
#include "Profiler/ProfilerTypes.h"
#include "String/StringLiteral.h"

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
     * Executes the lambda in render thread and sleeps until the task is finished
     */
    static void execInRenderThreadAndWait(RenderEnqFuncType &&execFunc);

    /**
     * Executes the passed in lambda in render thread and terminates. Fire and forget tasks can be enqueued this way
     */
    template <
        CBE_PROFILER_STRLITERAL CommandName, CBE_PROFILER_STRLITERAL FileName, CBE_PROFILER_STRLITERAL FuncName, uint32 Line,
        typename LambdaType>
    FORCE_INLINE static void execInRenderingThread(LambdaType &&lambdaFunc)
    {
#if ENABLE_PROFILING
        static constexpr const CBEProfilerSrcLoc srcLoc{ .name = CommandName.value,
                                                         .function = FuncName.value,
                                                         .file = FileName.value,
                                                         .line = Line,
                                                         .color = CBE_PROFILER_COLOR(CBE_PROFILER_DEFAULT_COLOR) };
        constexpr const CBEProfilerSrcLoc *srcLocPtr = &srcLoc;
#else
        constexpr const CBEProfilerSrcLoc *srcLocPtr = nullptr;
#endif
        // As purpose of enqueue is to execute in render thread not postpone execution if already in render thread.
        execInRenderingThreadOrImmediate(std::forward<LambdaType>(lambdaFunc), srcLocPtr);
    }

    FORCE_INLINE static void flushWaitRenderThread()
    {
        execInRenderThreadAndWait([](IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *) {});
    }

private:
    static copat::NormalFuncAwaiter execInRenderingThreadOrImmediate(RenderEnqFuncType &&execFunc, const CBEProfilerSrcLoc *srcLoc);
};

#define ENQUEUE_RENDER_COMMAND(CommandName)                                                                                                    \
    RenderThreadEnqueuer::execInRenderingThread<                                                                                               \
        CBE_PROFILER_CHAR(#CommandName), CBE_PROFILER_CHAR(__FILE__), CBE_PROFILER_CHAR(__func__), CBE_PROFILER_CHAR(__LINE__)>

#define ENQUEUE_RENDER_COMMAND_NODEBUG(CommandName, LambdaBody, ...)                                                                           \
    ENQUEUE_RENDER_COMMAND(CommandName)                                                                                                        \
    ([##__VA_ARGS__##](IRenderCommandList * cmdList, IGraphicsInstance * graphicsInstance, const GraphicsHelperAPI *graphicsHelper)##LambdaBody)

#define ASSERT_INSIDE_RENDERTHREAD()                                                                                                           \
    debugAssertf(                                                                                                                              \
        copat::JobSystem::get() && copat::JobSystem::get()->isInThread(copat::EJobThreadType::RenderThread), "Thread is not a render thread!"  \
    )