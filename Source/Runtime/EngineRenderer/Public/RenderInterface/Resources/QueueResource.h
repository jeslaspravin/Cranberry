#pragma once
#include "RenderInterface/Resources/GraphicsResources.h"
#include "Types/CoreTypes.h"

enum class EQueueFunction : uint8
{
    Generic = 0,
    Compute,
    Graphics,
    Transfer,
    Present
};

namespace EQueuePriority
{
    enum Enum : uint8
    {
        Low = 0,
        Medium,
        High,
        SuperHigh,
        MaxPriorityEnum
    };
}

class QueueResourceBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(QueueResourceBase,, GraphicsResource,)

public:

    virtual bool isValidQueue() const { return false; }

    void init() override;

};