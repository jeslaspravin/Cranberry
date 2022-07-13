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

void WidgetDrawContext::drawBox(
    ArrayView<Size2D> verts, ArrayView<Vector2D> coords, ArrayView<Color> color, ImageResourceRef *texture, QuantShortBox2D clip
)
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), color.cbegin(), color.cend());
    vertexCoord.insert(vertexCoord.end(), coords.cbegin(), coords.cend());
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(*texture);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(ArrayView<Size2D> verts, ArrayView<Color> color, QuantShortBox2D clip)
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), color.cbegin(), color.cend());
    vertexCoord.insert(vertexCoord.end(), 4, Vector2D::ZERO);
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(nullptr);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::drawBox(ArrayView<Size2D> verts, QuantShortBox2D clip)
{
    debugAssert(verts.size() == 4 && canAddMoreVerts(4));

    vertexColor.insert(vertexColor.end(), ColorConst::WHITE);
    vertexCoord.insert(vertexCoord.end(), 4, Vector2D::ZERO);
    vertices.insert(vertices.end(), verts.cbegin(), verts.cend());

    instanceTexture.emplace_back(nullptr);
    instanceClip.emplace_back(clip);
}

void WidgetDrawContext::addWaitCondition(SemaphoreRef *semaphore) { waitOnSemaphores.emplace_back(*semaphore); }

void WidgetDrawContext::beginLayer()
{
    if (layerAlt >= 0)
    {
        debugAssert(!altToVertRange[layerAlt].empty());
        altToVertRange[layerAlt].back().maxBound = vertices.size() - 1;
    }

    layerAlt++;
    std::vector<ValueRange<uint32>> &layerVerts = (altToVertRange.size() > layerAlt) ? altToVertRange[layerAlt] : altToVertRange.emplace_back();
    layerVerts.emplace_back(ValueRange<uint32>(vertices.size(), 0));
}

void WidgetDrawContext::endLayer()
{
    debugAssert(layerAlt >= 0);
    altToVertRange[layerAlt].back().maxBound = vertices.size() - 1;

    layerAlt--;
    if (layerAlt >= 0)
    {
        altToVertRange[layerAlt].emplace_back(ValueRange<uint32>(vertices.size(), 0));
    }
}

bool WidgetDrawContext::canAddMoreVerts(uint32 vertsCount) const { return (vertices.size() + vertsCount) < (~0u); }
