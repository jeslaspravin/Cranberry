#pragma once
#include "PlatformTypes.h"
#include "../String/String.h"

struct GenericAppInstance;

class GenericAppWindow {
protected:
	uint32 windowWidth;
	uint32 windowHeight;

	String windowName;
	GenericAppWindow* parentWindow = nullptr;
	std::vector<GenericAppWindow*> childWindows;

	bool isWindowed = false;
protected:
	virtual void resizeWindow() = 0;
public:

	void windowSize(uint32& width, uint32& height) const;
	void setWindowSize(const uint32& width, const uint32& height, bool updateResources);

	void setWindowName(const String& wndName);
	String getWindowName() const { return windowName; }

	virtual void createWindow(const GenericAppInstance* appInstance) {};
	virtual void destroyWindow();
	virtual bool isValidWindow() const= 0;
};