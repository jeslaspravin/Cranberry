/*!
 * \file WindowsAppWindow.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WindowsAppWindow.h"
#include "WindowsCommonHeaders.h"
#include "ApplicationInstance.h"
#include "ApplicationModule.h"
#include "PlatformAppInstanceBase.h"
#include "Logger/Logger.h"
#include "Math/Vector2.h"
#include "Types/CompilerDefines.h"

#include <set>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void WindowsAppWindow::createWindow(const ApplicationInstance *appInstance)
{
    HINSTANCE instanceHandle = (HINSTANCE)appInstance->platformApp->getPlatformAppInstance();
    // Setup application's default awareness, If no parent it is main window
    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/february/windows-with-c-write-high-dpi-apps-for-windows-8-1
    if (!parentWindow)
    {
        CALL_ONCE(
            []()
            {
                bool bSetDpiAwarness = ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
                fatalAssertf(bSetDpiAwarness, "DPI awareness setup failed");
            }
        );
    }

    WNDCLASS windowClass{};
    if (::GetClassInfo(instanceHandle, appInstance->getAppName().getChar(), &windowClass) == 0)
    {
        windowClass = {};
        windowClass.lpfnWndProc = &WindowProc;
        windowClass.hInstance = instanceHandle;
        windowClass.lpszClassName = appInstance->getAppName().getChar();

        RegisterClass(&windowClass);
    }

    dword style
        = bIsWindowed ? WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX : WS_POPUP | WS_MAXIMIZE;

    RECT windowRect{ 0, 0, (LONG)windowWidth, (LONG)windowHeight };

    ::AdjustWindowRect(&windowRect, style, false);

    windowHandle = CreateWindow(
        appInstance->getAppName().getChar(), windowName.getChar(), style, 0, 0, windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top, parentWindow ? (HWND) static_cast<WindowsAppWindow *>(parentWindow)->windowHandle : nullptr,
        nullptr, instanceHandle, this
    );

    if (windowHandle == nullptr)
    {
        LOG_ERROR("WindowsAppWindow", "Failed creating window, Error code {}", GetLastError());
        return;
    }

    ::ShowWindow((HWND)windowHandle, SW_SHOW);
    // Get window's dpi for a monitor
    windowDpiChanged(::GetDpiForWindow((HWND)windowHandle));
}

void WindowsAppWindow::updateWindow()
{
    // Since we are processing only raw inputs which are buffered only if not removed here
    const static std::set<uint32> IGNORED_MSGS{ WM_INPUT };

    auto peekMsgsLambda = [this](uint32 minFilter, uint32 maxFilter, uint32 removeFlag)
    {
        MSG msg;
        while (::PeekMessage(&msg, (HWND)windowHandle, minFilter, maxFilter, removeFlag) > 0)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    };

    uint32 ignoreFilterStart = 0;
    for (uint32 msgFilter : IGNORED_MSGS)
    {
        peekMsgsLambda(ignoreFilterStart, msgFilter - 1, PM_REMOVE);
        ignoreFilterStart = msgFilter + 1;
    }
    peekMsgsLambda(ignoreFilterStart, ~0u, PM_REMOVE);

    GenericAppWindow::updateWindow();
}

void WindowsAppWindow::destroyWindow()
{
    ::DestroyWindow((HWND)windowHandle);
    windowHandle = nullptr;

    // Doing after destroying native handle to allow processing native events like WM_ACTIVATE at WindowProc
    GenericAppWindow::destroyWindow();
}

bool WindowsAppWindow::isValidWindow() const { return windowHandle != nullptr; }

void WindowsAppWindow::pushEvent(uint32 eventType, LambdaFunction<void> &&function)
{
    accumulatedEvents[eventType] = std::forward<LambdaFunction<void>>(function);
}

void WindowsAppWindow::activateWindow() const
{
    if (onWindowActivated.isBound())
    {
        onWindowActivated.invoke();
    }
}

void WindowsAppWindow::deactivateWindow() const
{
    if (onWindowDeactived.isBound())
    {
        onWindowDeactived.invoke();
    }
}
void WindowsAppWindow::windowResizing(uint32 width, uint32 height) const
{
    if (onResize.isBound())
    {
        onResize.invoke(width, height);
    }
}

void WindowsAppWindow::windowDpiChanged(uint32 newDpi)
{
    CONST_EXPR int32 WINDOWS_DEFAULT_DPI = 96;
    dpiScaling = float(newDpi) / float(WINDOWS_DEFAULT_DPI);
}

void WindowsAppWindow::windowDestroyRequested() const
{
    if (onDestroyRequested.isBound())
    {
        onDestroyRequested.invoke();
    }
}

ShortRect WindowsAppWindow::windowClientRect() const
{
    ShortRect retVal(Short2(0), Short2(0));
    RECT clientArea;
    POINT clientOrigin{ 0, 0 };
    if (::GetClientRect((HWND)windowHandle, &clientArea) && ::ClientToScreen((HWND)windowHandle, &clientOrigin))
    {
        RECT clientBox = RECT{ .left = (clientArea.left + clientOrigin.x),
                               .top = (clientArea.top + clientOrigin.y),
                               .right = (clientArea.right + clientOrigin.x),
                               .bottom = (clientArea.bottom + clientOrigin.y) };
        fatalAssertf(
            clientBox.left < 0xFFFF && clientBox.top < 0xFFFF && clientBox.right < 0xFFFF && clientBox.bottom < 0xFFFF,
            "Window client area(lefttop=[{} {}] rightbottom=[{} {}]) exceeded capacity of int16 change to int32 rectangle", clientBox.left,
            clientBox.top, clientBox.right, clientBox.bottom
        );
        retVal.minBound = Short2(int16(clientBox.left), int16(clientBox.top));
        retVal.maxBound = Short2(int16(clientBox.right), int16(clientBox.bottom));
    }
    return retVal;
}

ShortRect WindowsAppWindow::windowRect() const
{
    ShortRect retVal(Short2(0), Short2(0));
    RECT windowRect;
    if (::GetWindowRect((HWND)windowHandle, &windowRect))
    {
        fatalAssertf(
            windowRect.left < 0xFFFF && windowRect.top < 0xFFFF && windowRect.right < 0xFFFF && windowRect.bottom < 0xFFFF,
            "Window rect area(lefttop=[{} {}] rightbottom=[{} {}]) exceeded capacity of int16 change to int32 rectangle", windowRect.left,
            windowRect.top, windowRect.right, windowRect.bottom
        );

        retVal.minBound = Short2(int16(windowRect.left), int16(windowRect.top));
        retVal.maxBound = Short2(int16(windowRect.right), int16(windowRect.bottom));
    }
    return retVal;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        const WindowsAppWindow *const windowPtr
            = reinterpret_cast<const WindowsAppWindow *>(reinterpret_cast<LPCREATESTRUCTA>(lParam)->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowPtr));
        LOG("WindowsAppWindow", "Created window {}", windowPtr->getWindowName().getChar());
        return 0;
    }
    case WM_DESTROY:
    {
        const WindowsAppWindow *const windowPtr = reinterpret_cast<const WindowsAppWindow *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        LOG("WindowsAppWindow", "Destroying window {}", windowPtr->getWindowName().getChar());
        return 0;
    }

    case WM_CLOSE:
    {
        WindowsAppWindow *const windowPtr = reinterpret_cast<WindowsAppWindow *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        LOG("WindowsAppWindow", "Quiting window {}", windowPtr->getWindowName().getChar());

        // This will trigger window destroyed event which can be used to kill engine
        windowPtr->pushEvent(
            WM_CLOSE,
            [windowPtr]()
            {
                windowPtr->windowDestroyRequested();
            }
        );
        return 0;
    }
    case WM_ACTIVATE:
    {
        WindowsAppWindow *const windowPtr = reinterpret_cast<WindowsAppWindow *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (windowPtr)
        {
            if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
            {
                windowPtr->activateWindow();
            }
            else
            {
                windowPtr->deactivateWindow();
            }
            return 0;
        }
        break;
    }
    case WM_DPICHANGED:
    {
        WindowsAppWindow *windowPtr = reinterpret_cast<WindowsAppWindow *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (windowPtr && LOWORD(wParam) > 0 && HIWORD(wParam) > 0)
        {
            windowPtr->windowDpiChanged(Math::max(LOWORD(wParam), HIWORD(wParam)));
            RECT &windowRect = *reinterpret_cast<RECT *>(lParam);
            SetWindowPos(
                hwnd, NULL, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                SWP_NOACTIVATE | SWP_NOZORDER
            );
        }
    }
    case WM_SIZE:
    {
        WindowsAppWindow *const windowPtr = reinterpret_cast<WindowsAppWindow *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (!windowPtr)
        {
            break;
        }

        if ((wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) && LOWORD(lParam) > 0 && HIWORD(lParam) > 0)
        {
            windowPtr->pushEvent(
                WM_SIZE,
                [windowPtr, lParam]()
                {
                    LOG("WindowsAppWindow", "Window {} Resized ({}, {})", windowPtr->getWindowName().getChar(), LOWORD(lParam), HIWORD(lParam));
                    windowPtr->windowResizing(LOWORD(lParam), HIWORD(lParam));
                }
            );
            return 0;
        }
        else if (wParam == SIZE_MINIMIZED)
        {
            windowPtr->pushEvent(
                WM_SIZE,
                [windowPtr, lParam]()
                {
                    LOG_DEBUG("WindowsAppWindow", "Window {} Minimized", windowPtr->getWindowName().getChar());
                    debugAssert(LOWORD(lParam) == 0 && HIWORD(lParam) == 0);
                    windowPtr->windowResizing(0, 0);
                }
            );
            return 0;
        }
        break;
    }
    default:
        break;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

WindowHandle WindowsAppWindow::getWindowUnderPoint(Short2 point)
{
    ::POINT pt;
    pt.x = point.x;
    pt.y = point.y;
    HWND wnd = ::WindowFromPoint(pt);
    if (wnd == NULL)
    {
        return wnd;
    }

    while (HWND childWnd = ::ChildWindowFromPoint(wnd, pt))
    {
        if (wnd == childWnd)
        {
            break;
        }
        wnd = childWnd;
    }
    return wnd;
}