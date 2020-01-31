#include "GenericAppWindow.h"



void GenericAppWindow::windowSize(uint32& width, uint32& height) const
{
	width = windowWidth;
	height = windowHeight;
}

void GenericAppWindow::setWindowSize(const uint32& width, const uint32& height)
{
	windowWidth = width;
	windowHeight = height;
	resizeWindow();
}
