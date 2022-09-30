/*!
 * \file CommandBuffer.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "Types/CoreTypes.h"

#include <vector>

enum class ECmdState
{
    Idle,      // Not recorded idle state
    Recording, // Between begin and end recording
    RenderPass,
    Recorded, // Recorded idle state after end recording before submit
    Submitted
};

struct SemaphoreSubmitInfo
{
    SemaphoreRef semaphore;
    // Pipeline Stages that are recorded in this command buffer that waits on the corresponding
    // semaphore
    uint64 stages;
};
struct TimelineSemaphoreSubmitInfo
{
    TimelineSemaphoreRef semaphore;
    // Pipeline Stages that are recorded in this command buffer that waits on the corresponding
    // semaphore
    uint64 stages;
    uint64 value;
};

// This struct is only for advanced usage else use cmd buffer based version CommandSubmitInfo2, If
// submitted with this way semaphores and fences has to be managed manually
struct CommandSubmitInfo
{
public:
    std::vector<const GraphicsResource *> cmdBuffers;
    std::vector<SemaphoreSubmitInfo> waitOn;
    std::vector<TimelineSemaphoreSubmitInfo> waitOnTimelines;
    std::vector<SemaphoreSubmitInfo> signalSemaphores;
    std::vector<TimelineSemaphoreSubmitInfo> signalTimelines;
};

struct CommandSubmitInfo2
{
public:
    std::vector<const GraphicsResource *> cmdBuffers;
    // All the cmd buffers will be waiting at top of pipeline for below buffers use with caution
    std::vector<const GraphicsResource *> waitOnCmdBuffers;
};