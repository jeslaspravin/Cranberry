/*!
 * \file IRHIModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Modules/IModuleBase.h"
#include "EngineRendererExports.h"

class IGraphicsInstance;
class GraphicsHelperAPI;

class ENGINERENDERER_EXPORT IRHIModule : public IModuleBase
{
public:
    virtual IGraphicsInstance* createGraphicsInstance() = 0;
    virtual void destroyGraphicsInstance() = 0;
    virtual const GraphicsHelperAPI* getGraphicsHelper() const = 0;
};