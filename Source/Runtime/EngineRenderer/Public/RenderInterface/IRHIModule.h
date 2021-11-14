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