#pragma once
#include "PlatformTypes.h"

struct GenericAppInstance;

class GenericAppWindow {
protected:
	uint32 windowWidth;
	uint32 windowHeight;

protected:
	virtual void resizeWindow() = 0;
public:

	void windowSize(uint32& width, uint32& height) const;
	void setWindowSize(const uint32& width, const uint32& height);


	virtual void createWindow(GenericAppInstance* appInstance) = 0;
};