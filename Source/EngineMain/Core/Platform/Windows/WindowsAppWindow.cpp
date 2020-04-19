#include "WindowsAppWindow.h"
#include "WindowsAppInstance.h"
#include "../../Logger/Logger.h"
#include "../../Engine/GameEngine.h"
#include "../../../RenderInterface/Resources/GenericWindowCanvas.h"

void WindowsAppWindow::resizeWindow()
{
    gEngine->getApplicationInstance()->appWindowManager.getWindowCanvas(this)->reinitResources();
}

LRESULT CALLBACK WindowProc(HWND   hwnd,UINT   uMsg,WPARAM wParam,LPARAM lParam);

void WindowsAppWindow::createWindow(const GenericAppInstance* appInstance)
{
    GenericAppWindow::createWindow(appInstance);

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

    dword style = isWindowed ? WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU : WS_POPUP;

    RECT windowRect{ 0,0,(LONG)windowWidth,(LONG)windowHeight };

    AdjustWindowRect(&windowRect, style, false);
    
    windowsHandle = CreateWindowA(windowName.getChar(), windowName.getChar(), style
        , 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        parentWindow? static_cast<WindowsAppWindow*>(parentWindow)->windowsHandle : NULL, NULL, instanceHandle, this);

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
}

bool WindowsAppWindow::isValidWindow() const
{
    return windowsHandle != nullptr && windowsHandle != NULL; 
}

void WindowsAppWindow::destroyWindow()
{
    GenericAppWindow::destroyWindow();

    DestroyWindow(windowsHandle);
    windowsHandle = NULL;
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
    default:
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}