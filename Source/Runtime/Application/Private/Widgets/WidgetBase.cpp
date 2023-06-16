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
    ArrayView<UInt2> verts, ArrayView<Vector2> coords, ArrayView<Color> colors, ImageResourceRef texture, ShortRect clip
) noexcept
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), colors.cbegin(), colors.cend());
    vertexCoord.insert(vertexCoord.end(), coords.cbegin(), coords.cend());
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(texture);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(ArrayView<UInt2> verts, ArrayView<Color> colors, ShortRect clip) noexcept
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), colors.cbegin(), colors.cend());
    vertexCoord.insert(vertexCoord.end(), 4, Vector2::ZERO);
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(nullptr);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(ArrayView<UInt2> verts, ShortRect clip) noexcept
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), 4, ColorConst::WHITE);
    vertexCoord.insert(vertexCoord.end(), 4, Vector2::ZERO);
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(nullptr);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(ShortRect box, ImageResourceRef texture, ShortRect clip, Color color /*= ColorConst::WHITE*/) noexcept
{
    UInt2 verts[4] = {
        box.minBound, {box.maxBound.x, box.minBound.y},
         box.maxBound, {box.minBound.x, box.maxBound.y}
    };
    Vector2 vertCoords[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    Color colors[4] = { color, color, color, color };

    drawBox(verts, vertCoords, colors, texture, clip);
}

void WidgetDrawContext::drawBox(ShortRect box, ImageResourceRef texture, ShortRect clip, ArrayView<Color> colors) noexcept
{
    UInt2 verts[4] = {
        box.minBound, {box.maxBound.x, box.minBound.y},
         box.maxBound, {box.minBound.x, box.maxBound.y}
    };
    Vector2 vertCoords[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    drawBox(verts, vertCoords, colors, texture, clip);
}

void WidgetDrawContext::addWaitCondition(SemaphoreRef semaphore) noexcept { waitOnSemaphores.emplace_back(semaphore); }

void WidgetDrawContext::beginLayer() noexcept
{
    if (layerAlt >= 0)
    {
        debugAssert(!altToVertRange[layerAlt].empty());
        altToVertRange[layerAlt].back().maxBound = vertices.empty() ? 0 : uint32(vertices.size() - 1);
        // If no vertices were added just remove the layer vertices
        if (!altToVertRange[layerAlt].back().isValidAABB()
            || altToVertRange[layerAlt].back().minBound == altToVertRange[layerAlt].back().maxBound)
        {
            altToVertRange[layerAlt].pop_back();
        }
    }

    layerAlt++;
    std::vector<ValueRange<uint32>> &layerVerts = (altToVertRange.size() > layerAlt) ? altToVertRange[layerAlt] : altToVertRange.emplace_back();
    layerVerts.emplace_back(ValueRange<uint32>(uint32(vertices.size()), 0));
}

void WidgetDrawContext::endLayer() noexcept
{
    debugAssert(layerAlt >= 0);
    altToVertRange[layerAlt].back().maxBound = vertices.empty() ? 0 : uint32(vertices.size() - 1);
    // If no vertices were added just remove the layer vertices
    if (!altToVertRange[layerAlt].back().isValidAABB() || altToVertRange[layerAlt].back().minBound == altToVertRange[layerAlt].back().maxBound)
    {
        altToVertRange[layerAlt].pop_back();
    }

    layerAlt--;
    if (layerAlt >= 0)
    {
        altToVertRange[layerAlt].emplace_back(ValueRange<uint32>(uint32(vertices.size()), 0));
    }
}

bool WidgetDrawContext::canAddMoreVerts(uint32 vertsCount) const noexcept { return (vertices.size() + vertsCount) < (~0u); }

void WidgetBase::rebuildWidgetGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree)
{
#if DEBUG_VALIDATIONS
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

#if DEBUG_VALIDATIONS
    debugAssertf(bRebuildingGeom, "Recursively calling rebuildWidgetGeometry of same widget!");
    bRebuildingGeom = false;
#endif
}
