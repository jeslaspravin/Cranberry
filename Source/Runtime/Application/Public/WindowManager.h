#pragma once

#include "Types/CoreTypes.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "ApplicationExports.h"

#include <map>
#include <set>

class GenericAppWindow;
class GenericWindowCanvas;
class InputSystem;

class APPLICATION_EXPORT WindowManager final
{
private:
    GenericAppWindow* appMainWindow = nullptr;
    GenericAppWindow* activeWindow = nullptr;

    struct ManagerData
    {
        WindowCanvasRef windowCanvas = nullptr;
    };

    std::map<GenericAppWindow*, ManagerData> windowsOpened;
    // For now Will be valid only inside pollWindows
    std::set<GenericAppWindow*> windowsToDestroy;

    void activateWindow(GenericAppWindow* window);
    void deactivateWindow(GenericAppWindow* window);
    void onWindowResize(uint32 width, uint32 height, GenericAppWindow* window);
    void requestDestroyWindow(GenericAppWindow* window);
public:

    GenericAppWindow* getMainWindow() const;
    WindowCanvasRef getWindowCanvas(GenericAppWindow* window) const;

    void init();
    void destroy();
    void destroyWindow(GenericAppWindow* window);
    void updateWindowCanvas();
    /*
    * Polls all windows for all events, Only events that are not handled in global event handler. 
    * Returns true if any active windows are there else false
    */
    bool pollWindows();
    // One time only function
    void postInitGraphicCore();
};
