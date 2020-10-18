#pragma once

class RenderSceneBase;

class RendererBase
{
private:
    RenderSceneBase* renderingScene;

protected:
    RendererBase(RenderSceneBase* scene)
        : renderingScene(scene)
    {}
};