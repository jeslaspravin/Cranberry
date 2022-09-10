/*!
 * \file WidgetRenderer.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Memory/SmartPointers.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"

#include <vector>
#include "Types/CoreDefines.h"

class WgWindow;
class WidgetDrawContext;
class IRenderCommandList;
class GraphicsHelperAPI;
class IGraphicsInstance;

class WidgetRenderer
{
public:
    static WidgetRenderer *createRenderer();

    virtual ~WidgetRenderer() = default;

    virtual void initialize() = 0;
    virtual void destroy() = 0;
    virtual void clearWindowState(const SharedPtr<WgWindow> &window) = 0;

    NODISCARD std::vector<SharedPtr<WgWindow>> drawWindowWidgets(const std::vector<SharedPtr<WgWindow>> &windows);
    void presentWindows(const std::vector<SharedPtr<WgWindow>> &windows);

protected:
    virtual void presentWindows(const std::vector<SharedPtr<WgWindow>> &windows, std::vector<WindowCanvasRef> swapchains) = 0;
    virtual void drawWindowWidgets(std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> &&drawingContexts) = 0;
};
