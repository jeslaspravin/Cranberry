#pragma once
#include <atomic>

#include "GraphicsResources.h"
#include "Types/CoreTypes.h"
#include "String/String.h"
#include "Types/Containers/ReferenceCountPtr.h"


class ENGINERENDERER_EXPORT GraphicsSyncResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsSyncResource, , GraphicsResource, )
private:
    std::atomic<uint32> refCounter;
protected:
    String resourceName;
public:

    virtual void waitForSignal() const {}
    virtual bool isSignaled() const { return false; }
    virtual void resetSignal() {}
    String getResourceName() const override;
    void setResourceName(const String& name) override;

    /* ReferenceCountPtr implementation */
    void addRef();
    void removeRef();
    uint32 refCount() const;
    /* Impl ends */
};

class ENGINERENDERER_EXPORT GraphicsSemaphore : public GraphicsSyncResource
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsSemaphore, , GraphicsSyncResource, )
};

class ENGINERENDERER_EXPORT GraphicsTimelineSemaphore : public GraphicsSyncResource
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

class ENGINERENDERER_EXPORT GraphicsFence : public GraphicsSyncResource
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsFence,,GraphicsSyncResource,)
};
using SemaphoreRef = ReferenceCountPtr<GraphicsSemaphore>;
using TimelineSemaphoreRef = ReferenceCountPtr<GraphicsTimelineSemaphore>;
using FenceRef = ReferenceCountPtr<GraphicsFence>;