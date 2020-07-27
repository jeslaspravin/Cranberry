#pragma once

#include <map>
#include "../Platform/PlatformTypes.h"

class GenericAppWindow;
class GenericWindowCanvas;
class InputSystem;

class WindowManager final
{
private:
    GenericAppWindow* appMainWindow = nullptr;
    GenericAppWindow* activeWindow = nullptr;

    struct ManagerData
    {
        GenericWindowCanvas* windowCanvas = nullptr;
    };

    std::map<GenericAppWindow*, ManagerData> windowsOpened;

    // TODO(Jeslas) : if ever supporting multi windows then this has to be redone
    InputSystem* inputSystem;

    void activateWindow(GenericAppWindow* window);
    void deactivateWindow(GenericAppWindow* window);
    void onWindowResize(uint32 width, uint32 height, GenericAppWindow* window);
    void onMouseMoved(uint32 xPos, uint32 yPos, GenericAppWindow* window);
public:

    GenericAppWindow* getMainWindow() const;
    GenericWindowCanvas* getWindowCanvas(GenericAppWindow* window) const;
    const InputSystem* getInputSystem() const;

    void initMain();
    void destroyMain();
    /*
    * Polls all windows for all events, Only events that are not handled in global event handler. 
    * Returns true if any active windows are there else false
    */
    bool pollWindows();
    // One time only function
    void postInitGraphicCore();
};

