#pragma once
#include "Types/CoreTypes.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"

#include <vector>

enum class ECmdState
{
    Idle, // Not recorded idle state
    Recording, // Between begin and end recording
    RenderPass,
    Recorded,// Recorded idle state after end recording before submit
    Submitted
};

// This struct is only for advanced usage else use cmd buffer based version CommandSubmitInfo2, If submitted with this way semaphores and fences has to be managed manually
struct CommandSubmitInfo
{
public:
    struct WaitInfo
    {
        SemaphoreRef waitOnSemaphore;
        // Pipeline Stages that are recorded in this command buffer that waits on the corresponding semaphore
        uint32 stagesThatWaits;
    };
    std::vector<const GraphicsResource*> cmdBuffers;
    std::vector<WaitInfo> waitOn;
    std::vector<SemaphoreRef> signalSemaphores;
};

struct CommandSubmitInfo2
{
public:
    std::vector<const GraphicsResource*> cmdBuffers;
    // All the cmd buffers will be waiting at top of pipeline for below buffers use with caution
    std::vector<const GraphicsResource*> waitOnCmdBuffers;
};