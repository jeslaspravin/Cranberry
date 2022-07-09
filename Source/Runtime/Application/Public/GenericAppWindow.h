/*!
 * \file GenericAppWindow.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "Math/Box.h"
#include "Reflections/Functions.h"
#include "String/String.h"
#include "Types/Delegates/Delegate.h"
#include "Types/Platform/PlatformTypes.h"

struct ApplicationInstance;

class APPLICATION_EXPORT GenericAppWindow
{
    friend class WindowManager;

protected:
    uint32 windowWidth;
    uint32 windowHeight;
    // Inverse of percent to scale the application interfaces. Example if dpi is 150% this will be
    // 100/150. This allows us to just multiply this value to render target resolution to get actual
    // resolution to work Example if we are rendering to 3840x2160 RT texts will be very small however
    // multiplying 150% scale factor gives virtual scaled resolution as 2560x1440
    float dpiScaling = 1.f;

    String windowName;
    // Not supporting multi window so this is irrelevant
    GenericAppWindow *parentWindow = nullptr;
    std::vector<GenericAppWindow *> childWindows;

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
    void windowSize(uint32 &width, uint32 &height) const;
    void setWindowSize(const uint32 &width, const uint32 &height, bool updateResources);
    FORCE_INLINE bool isMinimized() const { return windowWidth == 0 || windowHeight == 0; }

    float dpiScale() const { return dpiScaling; }

    /* Below set functions are initialize functions and are not usable after creating window */
    void setWindowMode(bool bIsFullScreen);
    void setWindowName(const String &wndName);
    const String &getWindowName() const { return windowName; }

    virtual void createWindow(const ApplicationInstance *appInstance) = 0;
    virtual void updateWindow();
    virtual void destroyWindow();
    virtual Rect windowClientRect() const = 0;
    virtual bool isValidWindow() const = 0;
    virtual WindowHandle getWindowHandle() const = 0;
};