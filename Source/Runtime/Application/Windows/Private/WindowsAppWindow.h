#pragma once
#include "GenericAppWindow.h"

class WindowsAppWindow final : public GenericAppWindow 
{
    
private:
    void* windowsHandle;

protected:
    // void resizeWindow() override;

public:
    void createWindow(const ApplicationInstance* appInstance) override;
    void updateWindow() override;
    void destroyWindow() override;
    bool isValidWindow() const override;
    void* getWindowHandle() const override { return windowsHandle; }

    void pushEvent(uint32 eventType, LambdaFunction<void> function);
    void activateWindow() const;
    void deactivateWindow() const;
    void windowResizing(uint32 width, uint32 height) const;
    Rect windowClientRect() const override;

};

namespace GPlatformInstances 
{
    typedef WindowsAppWindow PlatformAppWindow;
}