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

#include <map>

class ApplicationInstance;

class APPLICATION_EXPORT GenericAppWindow
{
    friend class WindowManager;

protected:
    uint32 windowWidth;
    uint32 windowHeight;
    // Percent to scale the application interfaces to match RT size. Example if dpi is 150% this will be
    // 150/100. This allows us to just divide this value to render target resolution to get actual
    // resolution to work Example if we are rendering to 3840x2160 RT texts will be very small however
    // dividing 150% scale factor gives virtual scaled resolution as 2560x1440
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

public:
    void windowSize(uint32 &width, uint32 &height) const
    {
        width = windowWidth;
        height = windowHeight;
    }
    void setWindowSize(uint32 width, uint32 height)
    {
        windowWidth = width;
        windowHeight = height;
    }

    FORCE_INLINE bool isMinimized() const { return windowWidth == 0 || windowHeight == 0; }

    float dpiScale() const { return dpiScaling; }

    /* Below set functions are initialize functions and are not usable after creating window */
    void setWindowMode(bool bIsFullScreen);
    void setWindowName(const TChar *wndName);
    void setParent(GenericAppWindow *window);
    const String &getWindowName() const { return windowName; }

    virtual void createWindow(const ApplicationInstance *appInstance) = 0;
    virtual void updateWindow();
    virtual void destroyWindow();
    // All Rect or positions are in unscaled desktop screen space
    virtual ShortRect windowClientRect() const = 0;
    virtual ShortRect windowRect() const = 0;
    virtual bool isValidWindow() const = 0;
    virtual WindowHandle getWindowHandle() const = 0;

    virtual ~GenericAppWindow() = default;

public:
    // Helpers
    static WindowHandle getWindowUnderPoint(Short2) { return nullptr; }
};