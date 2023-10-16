/*!
 * \file Renderer.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

class RenderSceneBase;

// TODO(Jeslas) : Remove this file once I am sure this is not needed
class RendererBase
{
private:
    RenderSceneBase *renderingScene;

protected:
    RendererBase(RenderSceneBase *scene)
        : renderingScene(scene)
    {}
};