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
#include "Math/Vector2D.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"

class WidgetDrawContext
{
private:
    std::vector<Color> vertexColor;
    std::vector<Vector2D> vertexCoord;
    std::vector<Short2D> vertices;
    // Below two are valid per quad(4 vertices)
    std::vector<ImageResourceRef> instanceTexture;
    std::vector<QuantShortBox2D> instanceClip;

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
        drawBox(ArrayView<Size2D> verts, ArrayView<Vector2D> coords, ArrayView<Color> colors, ImageResourceRef texture, QuantShortBox2D clip);
    APPLICATION_EXPORT void drawBox(ArrayView<Size2D> verts, ArrayView<Color> colors, QuantShortBox2D clip);
    APPLICATION_EXPORT void drawBox(ArrayView<Size2D> verts, QuantShortBox2D clip);
    APPLICATION_EXPORT void drawBox(QuantShortBox2D box, ImageResourceRef texture, QuantShortBox2D clip, Color color = ColorConst::WHITE);
    APPLICATION_EXPORT void drawBox(QuantShortBox2D box, ImageResourceRef texture, QuantShortBox2D clip, ArrayView<Color> colors);

    APPLICATION_EXPORT void addWaitCondition(SemaphoreRef semaphore);

    APPLICATION_EXPORT void beginLayer();
    APPLICATION_EXPORT void endLayer();

    FORCE_INLINE const std::vector<Color> &perVertexColor() const { return vertexColor; }
    FORCE_INLINE const std::vector<Short2D> &perVertexPos() const { return vertices; }
    FORCE_INLINE const std::vector<Vector2D> &perVertexUV() const { return vertexCoord; }

    FORCE_INLINE const std::vector<ImageResourceRef> &perQuadTexture() const { return instanceTexture; }
    FORCE_INLINE const std::vector<QuantShortBox2D> &perQuadClipping() const { return instanceClip; }

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
