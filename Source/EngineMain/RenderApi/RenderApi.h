#pragma once

class IGraphicsInstance;

class RenderApi {
    
    IGraphicsInstance* graphicsInstance;

public:

    void initialize();
    void destroy();
    IGraphicsInstance* getGraphicsInstance() const { return graphicsInstance; }
};
