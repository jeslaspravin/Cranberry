#pragma once
#include "../GenericAppWindow.h"
#include <windows.h>

class WindowsAppWindow:public GenericAppWindow {
	
private:
	HWND windowsHandle;

protected:
	void resizeWindow() override;

public:
	void createWindow(const GenericAppInstance* appInstance) override;
	void destroyWindow() override;

	HWND getWindowHandle() const { return windowsHandle; }
	bool isValidWindow() const override;

};

namespace GPlatformInstances {
	typedef WindowsAppWindow PlatformAppWindow;
}