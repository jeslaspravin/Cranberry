/*!
 * \file GenericWindowCanvas.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "Math/CoreMathTypedefs.h"

DEFINE_GRAPHICS_RESOURCE(GenericWindowCanvas)

uint32 GenericWindowCanvas::requestNextImage(
    SemaphoreRef *waitOnSemaphore, FenceRef *waitOnFence /*= nullptr*/)
{
    return 0;
}

void GenericWindowCanvas::addRef() { refCounter.fetch_add(1); }

void GenericWindowCanvas::removeRef()
{
    uint32 count = refCounter.fetch_sub(1);
    if (count == 1)
    {
        release();
        delete this;
    }
}

uint32 GenericWindowCanvas::refCount() const { return refCounter.load(); }

String GenericWindowCanvas::getResourceName() const
{
    // TODO(Jeslas)(Low) : return ownerWindow->getWindowName();
    // Does not matter as anyway marking graphics resource will be done with Window name directly
    return TCHAR("WindowCanvas");
}
