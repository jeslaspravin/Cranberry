#pragma once

class IGraphicsInstance;

class RenderApi {
private:
    IGraphicsInstance* graphicsInstance;

    void initAllShaders();
    void releaseAllShaders();
public:

    void initialize();
    void destroy();
    IGraphicsInstance* getGraphicsInstance() const { return graphicsInstance; }
};
