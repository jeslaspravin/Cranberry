#pragma once
#include "PlatformTypes.h"
#include "../String/String.h"
#include "../Types/Delegates/Delegate.h"

struct GenericAppInstance;

class GenericAppWindow {
    friend class WindowManager;
protected:
    uint32 windowWidth;
    uint32 windowHeight;

    String windowName;
    // Not supporting multi window so this is irrelevant
    GenericAppWindow* parentWindow = nullptr;
    std::vector<GenericAppWindow*> childWindows;

    bool bIsWindowed = true;

    SimpleDelegate onWindowActivated;
    SimpleDelegate onWindowDeactived;

protected:
    virtual void resizeWindow() = 0;
public:

    void windowSize(uint32& width, uint32& height) const;
    void setWindowSize(const uint32& width, const uint32& height, bool updateResources);

    /* Below set functions are initialize functions and are not usable after creating window */
    void setWindowMode(bool bIsFullScreen);
    void setWindowName(const String& wndName);
    const String& getWindowName() const { return windowName; }

    virtual void createWindow(const GenericAppInstance* appInstance) {};
    virtual void updateWindow() = 0;
    virtual void destroyWindow();
    virtual bool isValidWindow() const = 0;
};