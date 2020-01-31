#pragma once
#include "GraphicsResources.h"
#include "../../Core/Platform/PlatformTypes.h"

enum class EQueueFunction : uint8
{
	Generic = 0,
	Compute,
	Graphics,
	Transfer
};

enum class EQueuePriority : uint8
{
	Low = 0,
	Medium,
	High,
	SuperHigh,
	MaxPriorityEnum
};

class QueueResourceBase : public GraphicsResource
{
	DECLARE_GRAPHICS_RESOURCE(QueueResourceBase,, GraphicsResource,)


};