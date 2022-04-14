/*!
 * \file QueueResource.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/QueueResource.h"

DEFINE_GRAPHICS_RESOURCE(QueueResourceBase)

void QueueResourceBase::init()
{
    BaseType::init();
    reinitResources();
}
