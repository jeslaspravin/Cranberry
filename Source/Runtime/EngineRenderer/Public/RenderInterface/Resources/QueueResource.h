/*!
 * \file QueueResource.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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

class ENGINERENDERER_EXPORT QueueResourceBase : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(QueueResourceBase,, GraphicsResource,)

public:

    virtual bool isValidQueue() const { return false; }

    void init() override;

};