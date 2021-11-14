#pragma once

#include "EngineRendererExports.h"

class ENGINERENDERER_EXPORT IGraphicsInstance 
{
public:
    virtual void load() = 0;
    virtual void updateSurfaceDependents() = 0;
    virtual void unload() = 0;
    virtual void initializeCmds(class IRenderCommandList* commandList) = 0;
};
