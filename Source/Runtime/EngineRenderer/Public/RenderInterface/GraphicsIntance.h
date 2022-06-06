/*!
 * \file GraphicsIntance.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineRendererExports.h"

/**
 * Graphics hardware's instance for this application instance
 */
class ENGINERENDERER_EXPORT IGraphicsInstance
{
public:
    virtual void load() = 0;
    virtual void updateSurfaceDependents() = 0;
    virtual void unload() = 0;
    virtual void initializeCmds(class IRenderCommandList *commandList) = 0;
};
