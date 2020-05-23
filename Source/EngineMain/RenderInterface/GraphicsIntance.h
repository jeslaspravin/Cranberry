#pragma once

class IGraphicsInstance {

public:

    virtual void load() = 0;
    virtual void loadSurfaceDependents() = 0;
    virtual void unload() = 0;
    virtual void initializeCmds(class IRenderCommandList* commandList) = 0;
};
