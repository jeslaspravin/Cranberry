/*!
 * \file WorldViewport.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "ObjectPtrs.h"
#include "Classes/World.h"
#include "RenderInterface/Resources/BufferedResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/Resources/MemoryResources.h"

class GraphicsResource;
class WgRenderTarget;
class IRenderCommandList;
class GraphicsHelperAPI;
class IGraphicsInstance;

class ENGINECORE_EXPORT WorldViewport
{
public:
private:
    cbe::WeakObjPtr<cbe::World> world;
    using ShaderParamsRing = RingBufferedResource<std::pair<ImageResourceRef, ShaderParametersRef>, 4>;
    ShaderParamsRing resolveParams;

public:
    WorldViewport(cbe::World *inWorld)
        : world(inWorld)
    {}
    MAKE_TYPE_NONCOPY_NONMOVE(WorldViewport)

    void startSceneRender(Short2D viewportSize);
    /**
     * Clears and draws to widget back buffer, the necessary frame texture
     */
    void drawBackBuffer(
        QuantShortBox2D viewport, WgRenderTarget *rt, const GraphicsResource *cmdBuffer, IRenderCommandList *cmdList,
        IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
    );

private:
};
