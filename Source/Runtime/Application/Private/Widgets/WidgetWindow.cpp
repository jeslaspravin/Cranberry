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
    content = args.content;
    scaling = args.scaling;
    if (content)
    {
        setupParent(content, shared_from_this());
    }
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

void WgWindow::setContent(SharedPtr<WidgetBase> widget)
{
    if (content)
    {
        setupParent(content, nullptr);
    }
    content = widget;
    if (content)
    {
        setupParent(content, shared_from_this());
    }
}

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

void WgWindow::rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree)
{
    if (!content)
    {
        return;
    }

    content->rebuildGeometry(geomTree.add(WidgetGeom{ .widget = content }, 0), geomTree);
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

    auto nodeIdx = geomTree.getChildren(thisId)[0];
    const WidgetGeom &contentGeom = geomTree[nodeIdx];
    debugAssert(contentGeom.widget == content);
    context.beginLayer();
    content->drawWidget(clipBound.getIntersectionBox(contentGeom.box), nodeIdx, geomTree, context);
    context.endLayer();
}

void WgWindow::tick(float timeDelta)
{
    debugAssert(ownerWindow);

    allWidgetGeoms.clear();
    WidgetGeom windowGeom;
    windowGeom.widget = shared_from_this();
    windowGeom.box = QuantShortBox2D(Short2D(0), getWidgetSize());
    rebuildGeometry(allWidgetGeoms.add(windowGeom), allWidgetGeoms);

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

void WgWindow::mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) {}

void WgWindow::mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
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

SharedPtr<WgWindow> WidgetBase::findWidgetParentWindow(SharedPtr<WidgetBase> widget)
{
    if (!widget)
    {
        return nullptr;
    }
    SharedPtr<WidgetBase> rootWidget = widget;
    while (rootWidget->parentWidget != nullptr)
    {
        rootWidget = rootWidget->parentWidget;
    }
    return std::static_pointer_cast<WgWindow>(rootWidget);
}

float WidgetBase::getWidgetScaling(SharedPtr<WidgetBase> widget)
{
    if (!widget)
    {
        return 1.f;
    }
    return findWidgetParentWindow(widget)->getWidgetScaling();
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
        tempWidget = tempWidget->parentWidget;
    }

    std::reverse(widgetChain.begin(), widgetChain.end());
    return widgetChain;
}