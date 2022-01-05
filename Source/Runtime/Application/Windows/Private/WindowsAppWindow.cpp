/*!
 * \file WindowsAppWindow.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WindowsAppWindow.h"

#include <set>

#include "WindowsCommonHeaders.h"
#include "ApplicationInstance.h"
#include "ApplicationModule.h"
#include "PlatformAppInstanceBase.h"
#include "Logger/Logger.h"
#include "Math/Vector2D.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"

LRESULT CALLBACK WindowProc(HWND   hwnd,UINT   uMsg,WPARAM wParam,LPARAM lParam);

void WindowsAppWindow::createWindow(const ApplicationInstance* appInstance)
{
    HINSTANCE instanceHandle = (HINSTANCE)appInstance->platformApp->getPlatformAppInstance();
    WNDCLASSA windowClass{};
    if(GetClassInfoA(instanceHandle,windowName.getChar()
        ,&windowClass) == 0)
    {
        windowClass = {};
        windowClass.lpfnWndProc = &WindowProc;
        windowClass.hInstance = instanceHandle;
        windowClass.lpszClassName = windowName.getChar();

        RegisterClassA(&windowClass);
    }

    dword style = bIsWindowed ? WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX : WS_POPUP | WS_MAXIMIZE;

    RECT windowRect{ 0,0,(LONG)windowWidth,(LONG)windowHeight };

    AdjustWindowRect(&windowRect, style, false);
    
    windowsHandle = CreateWindowA(windowName.getChar(), windowName.getChar(), style
        , 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        parentWindow? (HWND)static_cast<WindowsAppWindow*>(parentWindow)->windowsHandle : nullptr, nullptr, instanceHandle, this);

    if (windowsHandle == nullptr)
    {
        Logger::error("WindowsAppWindow", "%s() : Failed creating window, Error code %d", __func__, GetLastError());
        return;
    }

    ShowWindow((HWND)windowsHandle, SW_SHOW);
}

void WindowsAppWindow::updateWindow()
{
    // Since we are processing only raw inputs which are buffered only if not removed here
    const static std::set<uint32> IGNORED_MSGS{ WM_INPUT };

    auto peekMsgsLambda = [this](uint32 minFilter, uint32 maxFilter, uint32 removeFlag)
    {
        MSG msg;
        while (PeekMessage(&msg, (HWND)windowsHandle, minFilter, maxFilter, removeFlag) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    };

    uint32 ignoreFilterStart = 0;
    for (const uint32& msgFilter : IGNORED_MSGS)
    {
        peekMsgsLambda(ignoreFilterStart, msgFilter - 1, PM_REMOVE);
        ignoreFilterStart = msgFilter + 1;
    }
    peekMsgsLambda(ignoreFilterStart, ~0u, PM_REMOVE);

    GenericAppWindow::updateWindow();
}

void WindowsAppWindow::destroyWindow()
{
    GenericAppWindow::destroyWindow();

    DestroyWindow((HWND)windowsHandle);
    windowsHandle = nullptr;
}


bool WindowsAppWindow::isValidWindow() const
{
    return windowsHandle != nullptr && windowsHandle != nullptr; 
}

void WindowsAppWindow::pushEvent(uint32 eventType, LambdaFunction<void> function)
{
    accumulatedEvents[eventType] = function;
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

void WindowsAppWindow::windowDestroyRequested() const
{
    if (onDestroyRequested.isBound())
    {
        onDestroyRequested.invoke();
    }
}

Rect WindowsAppWindow::windowClientRect() const
{
    Rect retVal(Vector2D::ZERO, Vector2D::ZERO);
    RECT clientArea;
    POINT clientOrigin{0, 0};
    if (GetClientRect((HWND)windowsHandle, &clientArea) && ClientToScreen((HWND)windowsHandle, &clientOrigin))
    {
        retVal.minBound = Vector2D(float(clientArea.left + clientOrigin.x), float(clientArea.top + clientOrigin.y));
        retVal.maxBound = Vector2D(float(clientArea.right + clientOrigin.x), float(clientArea.bottom + clientOrigin.y));
    }
    return retVal;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        const WindowsAppWindow* const windowPtr = reinterpret_cast<const WindowsAppWindow*>(
            reinterpret_cast<LPCREATESTRUCTA>(lParam)->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowPtr));
        Logger::log("WindowsAppWindow", "%s() : Created window %s", __func__, windowPtr->getWindowName().getChar());
        return 0;
    }        
    case WM_DESTROY:
    {        
        const WindowsAppWindow* const windowPtr = reinterpret_cast<const WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        Logger::log("WindowsAppWindow", "%s() : Destroying window %s", __func__, windowPtr->getWindowName().getChar());
        return 0;
    }

    case WM_CLOSE:
    {
        WindowsAppWindow* const windowPtr = reinterpret_cast<WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        Logger::log("WindowsAppWindow", "%s() : Quiting window %s", __func__, windowPtr->getWindowName().getChar());

        // This will trigger window destroyed event which can be used to kill engine
        windowPtr->pushEvent(WM_CLOSE, [windowPtr]() 
            {
                windowPtr->windowDestroyRequested();
            });
        return 0;
    }
    case WM_ACTIVATEAPP:
    {
        WindowsAppWindow* const windowPtr = reinterpret_cast<WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (windowPtr)
        {
            if (wParam == TRUE)
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
    case WM_SIZE:
    {
        WindowsAppWindow* const windowPtr = reinterpret_cast<WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (windowPtr && (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) && LOWORD(lParam) > 0 && HIWORD(lParam) > 0)
        {
            windowPtr->pushEvent(WM_ACTIVATEAPP, [windowPtr, lParam]() 
                {
                    Logger::log("WindowsAppWindow", "%s() : Resizing window %s ( %d, %d )", __func__, windowPtr->getWindowName().getChar(), LOWORD(lParam), HIWORD(lParam));
                    windowPtr->windowResizing(LOWORD(lParam), HIWORD(lParam));
                });
            return 0;
        }
        break;
    }
    default:
        break;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}