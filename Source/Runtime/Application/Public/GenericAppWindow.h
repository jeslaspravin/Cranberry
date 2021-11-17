#pragma once
#include "String/String.h"
#include "Types/Delegates/Delegate.h"
#include "Math/Box.h"
#include "Reflections/Functions.h"
#include "ApplicationExports.h"

struct ApplicationInstance;

class APPLICATION_EXPORT GenericAppWindow 
{
    friend class WindowManager;
protected:
    uint32 windowWidth;
    uint32 windowHeight;

    String windowName;
    // Not supporting multi window so this is irrelevant
    GenericAppWindow* parentWindow = nullptr;
    std::vector<GenericAppWindow*> childWindows;

    bool bIsWindowed = true;
    // Using map to avoid more than one message of same type to events(like multiple resize per frame
    std::map<uint32, LambdaFunction<void>> accumulatedEvents;

    using ScreenDimDelegate = Delegate<uint32, uint32>;

    // TODO(Jeslas) : Rework this delegate based direct events(Hard to extend)
    SimpleDelegate onWindowActivated;
    SimpleDelegate onWindowDeactived;
    SimpleDelegate onDestroyRequested;
    ScreenDimDelegate onResize;
protected:
    virtual void updateWindowResources();
public:
    void windowSize(uint32& width, uint32& height) const;
    void setWindowSize(const uint32& width, const uint32& height, bool updateResources);

    /* Below set functions are initialize functions and are not usable after creating window */
    void setWindowMode(bool bIsFullScreen);
    void setWindowName(const String& wndName);
    const String& getWindowName() const { return windowName; }

    virtual void createWindow(const ApplicationInstance* appInstance) = 0;
    virtual void updateWindow();
    virtual void destroyWindow();
    virtual Rect windowClientRect() const = 0;
    virtual bool isValidWindow() const = 0;
    virtual void* getWindowHandle() const = 0;
};