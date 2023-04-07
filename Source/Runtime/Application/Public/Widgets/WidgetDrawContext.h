/*!
 * \file WidgetDrawContext.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "Math/Box.h"
#include "Types/Colors.h"
#include "Math/Vector2.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"

class WidgetDrawContext
{
private:
    std::vector<Color> vertexColor;
    std::vector<Vector2> vertexCoord;
    std::vector<Short2> vertices;
    // Below two maps one per quad(4 vertices)
    std::vector<ImageResourceRef> instanceTexture;
    std::vector<ShortRect> instanceClip;

    std::vector<SemaphoreRef> waitOnSemaphores;
    /**
     * Maps each range of vertices that can be draw at same depth to its layer, Higher layer will be drawn on top of lower layer
     */
    std::vector<std::vector<ValueRange<uint32>>> altToVertRange;

    int32 layerAlt = -1;

public:
    /**
     * Vertices must be clockwise from viewer POV
     *
     *  v1------v2
     *  |        |
     *  |        |
     *  v4------v3
     */
    APPLICATION_EXPORT void
    drawBox(ArrayView<UInt2> verts, ArrayView<Vector2> coords, ArrayView<Color> colors, ImageResourceRef texture, ShortRect clip);
    APPLICATION_EXPORT void drawBox(ArrayView<UInt2> verts, ArrayView<Color> colors, ShortRect clip);
    APPLICATION_EXPORT void drawBox(ArrayView<UInt2> verts, ShortRect clip);
    APPLICATION_EXPORT void drawBox(ShortRect box, ImageResourceRef texture, ShortRect clip, Color color = ColorConst::WHITE);
    APPLICATION_EXPORT void drawBox(ShortRect box, ImageResourceRef texture, ShortRect clip, ArrayView<Color> colors);

    APPLICATION_EXPORT void addWaitCondition(SemaphoreRef semaphore);

    APPLICATION_EXPORT void beginLayer();
    APPLICATION_EXPORT void endLayer();

    FORCE_INLINE const std::vector<Color> &perVertexColor() const { return vertexColor; }
    FORCE_INLINE const std::vector<Short2> &perVertexPos() const { return vertices; }
    FORCE_INLINE const std::vector<Vector2> &perVertexUV() const { return vertexCoord; }

    FORCE_INLINE const std::vector<ImageResourceRef> &perQuadTexture() const { return instanceTexture; }
    FORCE_INLINE const std::vector<ShortRect> &perQuadClipping() const { return instanceClip; }

    FORCE_INLINE const std::vector<SemaphoreRef> &allWaitOnSemaphores() const { return waitOnSemaphores; }

    // Layers at higher indices appear on top of ones below
    FORCE_INLINE const auto &allLayerVertRange() const
    {
        debugAssertf(layerAlt == -1, "Getting all layer vertex range before all endLayer()");
        return altToVertRange;
    }

private:
    bool canAddMoreVerts(uint32 vertsCount) const;
};
