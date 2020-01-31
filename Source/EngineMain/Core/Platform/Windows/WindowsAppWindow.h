#pragma once
#include "../GenericAppWindow.h"
#include <windows.h>
#include "WindowsAppInstance.h"

class WindowsAppWindow:public GenericAppWindow {
	
	friend WindowsAppInstance;

private:
	HWND windowsHandle;

protected:
	void resizeWindow() override;

public:
	void createWindow(GenericAppInstance* appInstance) override;

};

namespace GPlatformInstances {
	typedef WindowsAppWindow PlatformAppWindow;
}