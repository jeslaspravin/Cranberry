/*!
 * \file Renderer.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

class RenderSceneBase;

class RendererBase
{
private:
    RenderSceneBase *renderingScene;

protected:
    RendererBase(RenderSceneBase *scene)
        : renderingScene(scene)
    {}
};