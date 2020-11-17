#pragma once
#include "../../Core/Platform/PlatformTypes.h"

#include <vector>

class GraphicsSemaphore;
class GraphicsResource;

enum class ECmdState
{
    Idle, // Not recorded idle state
    Recording, // Between begin and end recording
    Recorded,// Recorded idle state after end recording before submit
    Submitted
};

struct CommandSubmitInfo
{
public:
    struct WaitInfo
    {
        GraphicsSemaphore* waitOnSemaphore;
        // Pipeline Stages that are recorded in this command buffer that waits on the corresponding semaphore
        uint32 stagesThatWaits;
    };
    std::vector<const GraphicsResource*> cmdBuffers;
    std::vector<WaitInfo> waitOn;
    std::vector<GraphicsSemaphore*> signalSemaphores;
};