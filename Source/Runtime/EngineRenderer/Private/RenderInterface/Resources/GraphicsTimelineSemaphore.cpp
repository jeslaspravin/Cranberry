/*!
 * \file GraphicsTimelineSemaphore.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/GraphicsSyncResource.h"


void GraphicsTimelineSemaphore::waitForSignal() const
{
    if (currentValue() > 0)
        return;

    waitForSignal(1);
}

bool GraphicsTimelineSemaphore::isSignaled() const
{
    return isSignaled(1);
}

void GraphicsTimelineSemaphore::resetSignal()
{
    resetSignal(0);
}
