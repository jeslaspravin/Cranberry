/*!
 * \file GraphicsSyncResource.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/GraphicsSyncResource.h"

DEFINE_GRAPHICS_RESOURCE(GraphicsSyncResource)

void GraphicsSyncResource::addRef() { refCounter.fetch_add(1); }

void GraphicsSyncResource::removeRef()
{
    uint32 count = refCounter.fetch_sub(1);
    if (count == 1)
    {
        release();
        delete this;
    }
}

uint32 GraphicsSyncResource::refCount() const { return refCounter.load(); }

String GraphicsSyncResource::getResourceName() const { return resourceName; }

void GraphicsSyncResource::setResourceName(const String &name) { resourceName = name; }

DEFINE_GRAPHICS_RESOURCE(GraphicsFence)
DEFINE_GRAPHICS_RESOURCE(GraphicsSemaphore)
DEFINE_GRAPHICS_RESOURCE(GraphicsTimelineSemaphore)
