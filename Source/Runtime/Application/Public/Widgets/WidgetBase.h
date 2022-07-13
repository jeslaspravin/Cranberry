/*!
 * \file WidgetBase.h
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
#include "InputSystem/Keys.h"
#include "Types/Containers/FlatTree.h"
#include "Memory/SmartPointers.h"
#include "Math/Box.h"
#include "Math/Vector2D.h"
#include "Types/Colors.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"

class InputSystem;
class WidgetBase;
class WgWindow;

struct WidgetGeom
{
    SharedPtr<WidgetBase> widget = nullptr;
    QuantShortBox2D box;
};
using WidgetGeomTree = FlatTree<WidgetGeom, uint32>;
using WidgetGeomId = WidgetGeomTree::NodeIdx;

class WidgetDrawContext
{
private:
    std::vector<Color> vertexColor;
    std::vector<Vector2D> vertexCoord;
    std::vector<Short2D> vertices;
    std::vector<ImageResourceRef> instanceTexture;
    std::vector<QuantShortBox2D> instanceClip;

    std::vector<SemaphoreRef> waitOnSemaphores;
    /**
     * Maps each range of vertices that can be draw at same depth to its layer, Higher layer will be drawn on top of lower layer
     */
    std::vector<std::vector<ValueRange<uint32>>> altToVertRange;

    int32 layerAlt = -1;

public:
    APPLICATION_EXPORT void
        drawBox(ArrayView<Size2D> verts, ArrayView<Vector2D> coords, ArrayView<Color> color, ImageResourceRef *texture, QuantShortBox2D clip);
    APPLICATION_EXPORT void drawBox(ArrayView<Size2D> verts, ArrayView<Color> color, QuantShortBox2D clip);
    APPLICATION_EXPORT void drawBox(ArrayView<Size2D> verts, QuantShortBox2D clip);

    APPLICATION_EXPORT void addWaitCondition(SemaphoreRef *semaphore);

    APPLICATION_EXPORT void beginLayer();
    APPLICATION_EXPORT void endLayer();

    FORCE_INLINE const std::vector<Color> &perVertexColor() const { return vertexColor; }
    FORCE_INLINE const std::vector<Short2D> &perVertexPos() const { return vertices; }
    FORCE_INLINE const std::vector<Vector2D> &perVertexUV() const { return vertexCoord; }

    FORCE_INLINE const std::vector<ImageResourceRef> &perQuadTexture() const { return instanceTexture; }
    FORCE_INLINE const std::vector<QuantShortBox2D> &perQuadClipping() const { return instanceClip; }

    FORCE_INLINE const std::vector<SemaphoreRef> &allWaitOnSemaphores() const { return waitOnSemaphores; }

    // Layers at higher indices appear on top of ones below
    FORCE_INLINE const auto &allLayerVertRange() const { return altToVertRange; }

private:
    bool canAddMoreVerts(uint32 vertsCount) const;
};

enum class EInputHandleState
{
    Processed,
    NotHandled
};

class APPLICATION_EXPORT WidgetBase : public std::enable_shared_from_this<WidgetBase>
{
protected:
    SharedPtr<WidgetBase> parentWidget = nullptr;

public:
    virtual ~WidgetBase() = default;
    // Rebuild and draw widget is recursive. Each widget must call its sub widgets
    virtual void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) = 0;
    virtual void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) = 0;

    // below virtual functions are non recursive. Overrides do not have to call its child as children will be processed before parent
    virtual void tick(float timeDelta) = 0;
    virtual EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) = 0;
    virtual void mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) = 0;
    virtual void mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) = 0;
    virtual void mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) = 0;

    static SharedPtr<WgWindow> findWidgetParentWindow(SharedPtr<WidgetBase> widget);

protected:
    static float getWidgetScaling(SharedPtr<WidgetBase> widget);
    /**
     * Gets widget's geometry in this frame, Avoid calling this often as it traverses the geometry tree to find it
     */
    static WidgetGeom getWidgetGeom(SharedPtr<WidgetBase> widget);

    // From root window widget at 0 to finding widget at (n - 1)
    static std::vector<SharedPtr<WidgetBase>> getWidgetChain(SharedPtr<WidgetBase> widget);
    static void setupParent(SharedPtr<WidgetBase> childWidget, SharedPtr<WidgetBase> parentWidget) { childWidget->parentWidget = parentWidget; }
};