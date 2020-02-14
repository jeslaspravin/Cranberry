#pragma once
#include "GraphicsResources.h"
#include "../../Core/Memory/SmartPointers.h"
#include "../../Core/Platform/PlatformTypes.h"

class GenericAppWindow;
class GraphicsSemaphore;
class GraphicsFence;

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
    String getResourceName() const override;

	// Setup function must be use before calling init
	virtual void setWindow(GenericAppWindow* forWindow);

	virtual uint32 requestNextImage(SharedPtr<GraphicsSemaphore>* waitOnSemaphore, SharedPtr<GraphicsFence>* waitOnFence = nullptr);

};

