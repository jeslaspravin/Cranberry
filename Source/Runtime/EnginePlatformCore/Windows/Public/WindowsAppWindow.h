#pragma once
#include "GenericAppWindow.h"
#include "EnginePlatformCoreExports.h"

class ENGINEPLATFORMCORE_EXPORT WindowsAppWindow final : public GenericAppWindow 
{
    
private:
    void* windowsHandle;

protected:
    // void resizeWindow() override;

public:
    void createWindow(const GenericAppInstance* appInstance) override;
    void updateWindow() override;
    void destroyWindow() override;
    bool isValidWindow() const override;

    void* getWindowHandle() const { return windowsHandle; }

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