#pragma once
#include "../GenericAppWindow.h"
#include "WindowsCommonHeaders.h"

class WindowsAppWindow final : public GenericAppWindow 
{
    
private:
    HWND windowsHandle;

protected:
    void resizeWindow() override;

public:
    void createWindow(const GenericAppInstance* appInstance) override;
    void updateWindow() override;
    void destroyWindow() override;
    bool isValidWindow() const override;

    HWND getWindowHandle() const { return windowsHandle; }

    void pushEvent(uint32 eventType, LambdaFunction<void> function);
    void activateWindow() const;
    void deactivateWindow() const;
    void windowResizing(uint32 width, uint32 height) const;
    Rect windowClientRect() const override;

};

namespace GPlatformInstances {
    typedef WindowsAppWindow PlatformAppWindow;
}