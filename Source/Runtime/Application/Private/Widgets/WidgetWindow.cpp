/*!
 * \file WidgetWindow.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WidgetWindow.h"
#include "InputSystem/InputSystem.h"
#include "Widgets/WidgetDrawContext.h"

void WgWindow::construct(const WgArguments &args)
{
    ownerWindow = args.ownerWindow;
    scaling = args.scaling;
    setContent(args.content);

    // Insert root
    WidgetGeom windowGeom;
    windowGeom.widget = shared_from_this();
    windowGeom.box = QuantShortBox2D(Short2D(0), getWidgetSize());
    allWidgetGeoms.add(windowGeom);
}

void WgWindow::drawWidget(WidgetDrawContext &context)
{
#if DEV_BUILD
    std::vector<WidgetGeomTree::NodeIdx> roots;
    roots.reserve(1);
    allWidgetGeoms.getAllRoots(roots);
    debugAssert(roots.size() == 1 && roots[0] == 0);
#endif

    drawWidget(QuantShortBox2D(Short2D(0), getWidgetSize()), 0, allWidgetGeoms, context);
}

bool WgWindow::hasWidget(SharedPtr<WidgetBase> widget) const
{
    return widget.get() == this || (content && (widget == content || content->hasWidget(widget)));
}

void WgWindow::rebuildWindowGeoms()
{
    allWidgetGeoms.clear();
    WidgetGeom windowGeom;
    windowGeom.widget = shared_from_this();
    windowGeom.box = QuantShortBox2D(Short2D(0), getWidgetSize());
    rebuildWidgetGeometry(allWidgetGeoms.add(windowGeom), allWidgetGeoms);
}

void WgWindow::clearWindow()
{
    allWidgetGeoms.clear();
    content.reset();
    hoveringWidget.reset();
}

void WgWindow::setContent(SharedPtr<WidgetBase> widget) { content = widget; }

WidgetGeom WgWindow::findWidgetGeom(SharedPtr<WidgetBase> widget) const
{
    if (widget.get() == this)
    {
        return allWidgetGeoms[0];
    }
    std::vector<SharedPtr<WidgetBase>> widgetChain = getWidgetChain(widget);

    // ArrayView since have to skip this WgWindow for consistent looping
    WidgetGeomTree::NodeIdx currentLinkIdx = 0;
    for (SharedPtr<WidgetBase> chainLinkWidget : ArrayView<SharedPtr<WidgetBase>>(widgetChain.data(), widgetChain.size(), 1))
    {
        WidgetGeomTree::NodeIdx nextLinkIdx = currentLinkIdx;
        std::vector<WidgetGeomTree::NodeIdx> children;
        allWidgetGeoms.getChildren(children, currentLinkIdx, false);
        for (WidgetGeomTree::NodeIdx childIdx : children)
        {
            const WidgetGeom &widgetGeom = allWidgetGeoms[childIdx];
            if (widgetGeom.widget == chainLinkWidget)
            {
                nextLinkIdx = childIdx;
                break;
            }
        }
        // If nextLinkIdx is not updated to new index then it means widget is not found, So return empty
        if (nextLinkIdx == currentLinkIdx)
        {
            return {};
        }
        currentLinkIdx = nextLinkIdx;
    }

    return allWidgetGeoms[currentLinkIdx];
}

void WgWindow::rebuildGeometry(WidgetGeomId /*thisId*/, WidgetGeomTree &geomTree)
{
    if (!content)
    {
        return;
    }

    content->rebuildWidgetGeometry(geomTree.add(WidgetGeom{ .widget = content }, 0), geomTree);
    std::vector<WidgetGeomTree::NodeIdx> children;
    geomTree.getChildren(children, 0, true);

    for (WidgetGeomTree::NodeIdx childIdx : children)
    {
        const WidgetGeomTree::Node &childNode = geomTree.getNode(childIdx);
        WidgetGeom &childGeom = geomTree[childIdx];
        WidgetGeom &parentGeom = geomTree[childNode.parent];

        childGeom.box += parentGeom.box.minBound;
    }
}

void WgWindow::drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context)
{
    if (!content)
    {
        return;
    }
    std::vector<WidgetGeomId> children = geomTree.getChildren(thisId);
    if (children.empty())
    {
        // This happens only when we are deactivating all window of application before first ticking the window after setting window content
        return;
    }

    auto nodeIdx = children[0];
    const WidgetGeom &contentGeom = geomTree[nodeIdx];
    debugAssert(contentGeom.widget == content);
    context.beginLayer();
    content->drawWidget(clipBound.getIntersectionBox(contentGeom.box), nodeIdx, geomTree, context);
    context.endLayer();
}

void WgWindow::tick(float timeDelta)
{
    debugAssert(ownerWindow);

    rebuildWindowGeoms();

    // All inner most children will be at last
    std::vector<WidgetGeomTree::NodeIdx> children;
    allWidgetGeoms.getChildren(children, 0, true);

    for (auto rItr = children.rbegin(); rItr != children.rend(); ++rItr)
    {
        const WidgetGeom &widgetGeom = allWidgetGeoms[*rItr];
        debugAssert(widgetGeom.widget);
        widgetGeom.widget->tick(timeDelta);
    }
}

EInputHandleState WgWindow::inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem)
{
    // All inner most children will be at last
    std::vector<WidgetGeomTree::NodeIdx> children;
    allWidgetGeoms.getChildren(children, 0, true);
    for (auto rItr = children.rbegin(); rItr != children.rend(); ++rItr)
    {
        const WidgetGeom &widgetGeom = allWidgetGeoms[*rItr];
        debugAssert(widgetGeom.widget);
        if (widgetGeom.widget->inputKey(key, state, inputSystem) == EInputHandleState::Processed)
        {
            return EInputHandleState::Processed;
        }
    }
    return EInputHandleState::NotHandled;
}

EInputHandleState WgWindow::analogKey(AnalogStates::StateKeyType key, AnalogStates::StateInfoType state, const InputSystem *inputSystem)
{
    // All inner most children will be at last
    std::vector<WidgetGeomTree::NodeIdx> children;
    allWidgetGeoms.getChildren(children, 0, true);
    for (auto rItr = children.rbegin(); rItr != children.rend(); ++rItr)
    {
        const WidgetGeom &widgetGeom = allWidgetGeoms[*rItr];
        debugAssert(widgetGeom.widget);
        if (widgetGeom.widget->analogKey(key, state, inputSystem) == EInputHandleState::Processed)
        {
            return EInputHandleState::Processed;
        }
    }
    return EInputHandleState::NotHandled;
}

void WgWindow::mouseEnter(Short2D /*absPos*/, Short2D /*widgetRelPos*/, const InputSystem * /*inputSystem*/) {}

void WgWindow::mouseMoved(Short2D absPos, Short2D /*widgetRelPos*/, const InputSystem *inputSystem)
{
    // All inner most children will be at last
    std::vector<WidgetGeomTree::NodeIdx> children;
    allWidgetGeoms.getChildren(children, 0, true);

    WidgetGeom currentHoverGeom;
    for (auto rItr = children.rbegin(); rItr != children.rend(); ++rItr)
    {
        const WidgetGeom &widgetGeom = allWidgetGeoms[*rItr];
        debugAssert(widgetGeom.widget);

        if (widgetGeom.box.contains(mousePos))
        {
            currentHoverGeom = widgetGeom;
            break;
        }
    }
    if (currentHoverGeom.widget != hoveringWidget)
    {
        if (hoveringWidget)
        {
            WidgetGeom oldGeom = findWidgetGeom(hoveringWidget);
            debugAssert(oldGeom.widget);
            hoveringWidget->mouseLeave(absPos, absPos - oldGeom.box.minBound, inputSystem);
        }
        hoveringWidget = currentHoverGeom.widget;
        if (hoveringWidget)
        {
            hoveringWidget->mouseEnter(absPos, absPos - currentHoverGeom.box.minBound, inputSystem);
        }
    }
    mousePos = absPos;

    if (hoveringWidget)
    {
        hoveringWidget->mouseMoved(absPos, absPos - currentHoverGeom.box.minBound, inputSystem);
    }
}

void WgWindow::mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    if (hoveringWidget)
    {
        hoveringWidget->mouseLeave(absPos, widgetRelPos, inputSystem);
        hoveringWidget = nullptr;
    }
}

WidgetGeom WidgetBase::getWidgetGeom(SharedPtr<WidgetBase> widget)
{
    if (SharedPtr<WgWindow> windowWidget = findWidgetParentWindow(widget))
    {
        return windowWidget->findWidgetGeom(widget);
    }
    return {};
}

std::vector<SharedPtr<WidgetBase>> WidgetBase::getWidgetChain(SharedPtr<WidgetBase> widget)
{
    std::vector<SharedPtr<WidgetBase>> widgetChain;
    SharedPtr<WidgetBase> tempWidget = widget;
    while (tempWidget)
    {
        widgetChain.emplace_back(tempWidget);
        tempWidget = tempWidget->parentWidget.expired() ? nullptr : tempWidget->parentWidget.lock();
    }

    std::reverse(widgetChain.begin(), widgetChain.end());
    return widgetChain;
}