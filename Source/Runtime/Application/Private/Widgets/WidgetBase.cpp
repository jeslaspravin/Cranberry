/*!
 * \file WidgetBase.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WidgetBase.h"
#include "Widgets/WidgetDrawContext.h"

void WidgetDrawContext::drawBox(
    ArrayView<Size2D> verts, ArrayView<Vector2D> coords, ArrayView<Color> colors, ImageResourceRef texture, QuantShortBox2D clip
)
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), colors.cbegin(), colors.cend());
    vertexCoord.insert(vertexCoord.end(), coords.cbegin(), coords.cend());
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(texture);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(ArrayView<Size2D> verts, ArrayView<Color> colors, QuantShortBox2D clip)
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), colors.cbegin(), colors.cend());
    vertexCoord.insert(vertexCoord.end(), 4, Vector2D::ZERO);
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(nullptr);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(ArrayView<Size2D> verts, QuantShortBox2D clip)
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), 4, ColorConst::WHITE);
    vertexCoord.insert(vertexCoord.end(), 4, Vector2D::ZERO);
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(nullptr);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(QuantShortBox2D box, ImageResourceRef texture, QuantShortBox2D clip, Color color /*= ColorConst::WHITE*/)
{
    Size2D verts[4] = {
        box.minBound, {box.maxBound.x, box.minBound.y},
         box.maxBound, {box.minBound.x, box.maxBound.y}
    };
    Vector2D vertCoords[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    Color colors[4] = { color, color, color, color };

    drawBox(verts, vertCoords, colors, texture, clip);
}

void WidgetDrawContext::drawBox(QuantShortBox2D box, ImageResourceRef texture, QuantShortBox2D clip, ArrayView<Color> colors)
{
    Size2D verts[4] = {
        box.minBound, {box.maxBound.x, box.minBound.y},
         box.maxBound, {box.minBound.x, box.maxBound.y}
    };
    Vector2D vertCoords[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    drawBox(verts, vertCoords, colors, texture, clip);
}

void WidgetDrawContext::addWaitCondition(SemaphoreRef semaphore) { waitOnSemaphores.emplace_back(semaphore); }

void WidgetDrawContext::beginLayer()
{
    if (layerAlt >= 0)
    {
        debugAssert(!altToVertRange[layerAlt].empty());
        altToVertRange[layerAlt].back().maxBound = vertices.empty() ? 0 : vertices.size() - 1;
        // If no vertices were added just remove the layer vertices
        if (!altToVertRange[layerAlt].back().isValidAABB()
            || altToVertRange[layerAlt].back().minBound == altToVertRange[layerAlt].back().maxBound)
        {
            altToVertRange[layerAlt].pop_back();
        }
    }

    layerAlt++;
    std::vector<ValueRange<uint32>> &layerVerts = (altToVertRange.size() > layerAlt) ? altToVertRange[layerAlt] : altToVertRange.emplace_back();
    layerVerts.emplace_back(ValueRange<uint32>(vertices.size(), 0));
}

void WidgetDrawContext::endLayer()
{
    debugAssert(layerAlt >= 0);
    altToVertRange[layerAlt].back().maxBound = vertices.empty() ? 0 : vertices.size() - 1;
    // If no vertices were added just remove the layer vertices
    if (!altToVertRange[layerAlt].back().isValidAABB() || altToVertRange[layerAlt].back().minBound == altToVertRange[layerAlt].back().maxBound)
    {
        altToVertRange[layerAlt].pop_back();
    }

    layerAlt--;
    if (layerAlt >= 0)
    {
        altToVertRange[layerAlt].emplace_back(ValueRange<uint32>(vertices.size(), 0));
    }
}

bool WidgetDrawContext::canAddMoreVerts(uint32 vertsCount) const { return (vertices.size() + vertsCount) < (~0u); }

void WidgetBase::rebuildWidgetGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree)
{
#if DEBUG_BUILD
    debugAssertf(!bRebuildingGeom, "Recursively calling rebuildWidgetGeometry of same widget!");
    bRebuildingGeom = true;
#endif

    debugAssert(geomTree.isValid(thisId));
    WidgetGeomId parentId = geomTree.getNode(thisId).parent;
    if (geomTree.isValid(parentId))
    {
        parentWidget = geomTree[parentId].widget;
    }
    else
    {
        parentWidget.reset();
    }
    rebuildGeometry(thisId, geomTree);

#if DEBUG_BUILD
    debugAssertf(bRebuildingGeom, "Recursively calling rebuildWidgetGeometry of same widget!");
    bRebuildingGeom = false;
#endif
}
