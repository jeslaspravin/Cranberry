#pragma once
#include "GraphicsResources.h"

class GenericAppWindow;

// Wrapper for VkSurface and related kind of objects
class GenericWindowCanvas : public GraphicsResource
{
	DECLARE_GRAPHICS_RESOURCE(GenericWindowCanvas,,GraphicsResource,)

protected:
	GenericAppWindow* ownerWindow = nullptr;
public:
	virtual void init() override;
	virtual void reinitResources() override {}
	virtual void release() override;

	// Setup function must be use before calling init
	virtual void setWindow(GenericAppWindow* forWindow);
};

