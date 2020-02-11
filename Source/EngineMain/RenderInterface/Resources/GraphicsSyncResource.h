#pragma once
#include "GraphicsResources.h"
#include "../../Core/Platform/PlatformTypes.h"


class GraphicsSyncResource : public GraphicsResource
{
	DECLARE_GRAPHICS_RESOURCE(GraphicsSyncResource,,GraphicsResource,)

public:

	virtual void waitForSignal() const {}
	virtual bool isSignaled() const { return false; }
	virtual void resetSignal() {}
};

class GraphicsSemaphore : public GraphicsSyncResource
{
	DECLARE_GRAPHICS_RESOURCE(GraphicsSemaphore, , GraphicsSyncResource, )
};

class GraphicsTimelineSemaphore : public GraphicsSyncResource
{
	DECLARE_GRAPHICS_RESOURCE(GraphicsTimelineSemaphore, , GraphicsSyncResource, )

public:

	void waitForSignal() const override;
	bool isSignaled() const override;
	void resetSignal() override;


	virtual void waitForSignal(uint64 value) const {}
	virtual bool isSignaled(uint64 value) const { return false; }
	virtual void resetSignal(uint64 value) {}
	virtual uint64 currentValue() const { return 0; }
};

class GraphicsFence : public GraphicsSyncResource
{
	DECLARE_GRAPHICS_RESOURCE(GraphicsFence,,GraphicsSyncResource,)
};