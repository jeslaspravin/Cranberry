#include "WindowsAppInstance.h"
#include "WindowsAppWindow.h"

HWND WindowsAppInstance::getWindowHandle() const
{
	if (appWindow)
	{
		return static_cast<WindowsAppWindow*>(appWindow)->windowsHandle;
	}
	return nullptr;
}