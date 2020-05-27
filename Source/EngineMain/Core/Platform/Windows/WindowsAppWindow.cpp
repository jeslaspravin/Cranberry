#include "WindowsAppWindow.h"
#include "WindowsAppInstance.h"
#include "../../Logger/Logger.h"
#include "../../Engine/GameEngine.h"
#include "../../../RenderInterface/Resources/GenericWindowCanvas.h"

void WindowsAppWindow::resizeWindow()
{
    GenericWindowCanvas* windowCanvas = gEngine->getApplicationInstance()->appWindowManager.getWindowCanvas(this);
    if (windowCanvas)
    {
        Logger::debug("WindowsAppWindow", "%s() : Reiniting window canvas", __func__);
        windowCanvas->reinitResources();
    }
}

LRESULT CALLBACK WindowProc(HWND   hwnd,UINT   uMsg,WPARAM wParam,LPARAM lParam);

void WindowsAppWindow::createWindow(const GenericAppInstance* appInstance)
{
    HINSTANCE instanceHandle = static_cast<const WindowsAppInstance*>(appInstance)->windowsInstance;
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

    dword style = bIsWindowed ? WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX : WS_POPUP | WS_MAXIMIZE;

    RECT windowRect{ 0,0,(LONG)windowWidth,(LONG)windowHeight };

    AdjustWindowRect(&windowRect, style, false);
    
    windowsHandle = CreateWindowA(windowName.getChar(), windowName.getChar(), style
        , 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        parentWindow? static_cast<WindowsAppWindow*>(parentWindow)->windowsHandle : nullptr, nullptr, instanceHandle, this);

    if (windowsHandle == nullptr)
    {
        Logger::error("WindowsAppWindow", "%s() : Failed creating window, Error code %d", __func__, GetLastError());
        return;
    }

    ShowWindow(windowsHandle,SW_SHOW);
}

void WindowsAppWindow::updateWindow()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    GenericAppWindow::updateWindow();
}

bool WindowsAppWindow::isValidWindow() const
{
    return windowsHandle != nullptr && windowsHandle != nullptr; 
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

void WindowsAppWindow::destroyWindow()
{
    GenericAppWindow::destroyWindow();

    DestroyWindow(windowsHandle);
    windowsHandle = nullptr;
}

void WindowsAppWindow::windowResizing(uint32 width, uint32 height)
{
    if (onResize.isBound())
    {
        onResize.invoke(width, height);
    }
}

void WindowsAppWindow::pushEvent(uint32 eventType, LambdaFunction<void> function)
{
    accumulatedEvents[eventType] = function;
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
        break;
    }        
    case WM_DESTROY:
    {        
        const WindowsAppWindow* const windowPtr = reinterpret_cast<const WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        Logger::log("WindowsAppWindow", "%s() : Destroying window %s", __func__, windowPtr->getWindowName().getChar()); 
        break;
    }

    case WM_CLOSE:
    {
        const WindowsAppWindow* const windowPtr = reinterpret_cast<const WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        Logger::log("WindowsAppWindow", "%s() : Quiting window %s", __func__, windowPtr->getWindowName().getChar());

        if (gEngine && gEngine->getApplicationInstance()->appWindowManager.getMainWindow() == windowPtr)
        {
            gEngine->requestExit();
        }
        break;
    }
    case WM_ACTIVATEAPP:
    {
        WindowsAppWindow* const windowPtr = reinterpret_cast<WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (!windowPtr)
        {
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
        }
        if (wParam == TRUE)
        {
            windowPtr->activateWindow();
        }
        else
        {
            windowPtr->deactivateWindow();
        }
        break;
    }
    case WM_SIZE:
    {
        WindowsAppWindow* const windowPtr = reinterpret_cast<WindowsAppWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (windowPtr && (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) && LOWORD(lParam) > 0 && HIWORD(lParam) > 0)
        {
            windowPtr->pushEvent(WM_ACTIVATEAPP, { [windowPtr, lParam]() 
                {
                    Logger::log("WindowsAppWindow", "%s() : Resizing window %s ( %d, %d )", __func__, windowPtr->getWindowName().getChar(), LOWORD(lParam), HIWORD(lParam));
                    windowPtr->windowResizing(LOWORD(lParam), HIWORD(lParam));
                }});
            break;
        }
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    default:
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}