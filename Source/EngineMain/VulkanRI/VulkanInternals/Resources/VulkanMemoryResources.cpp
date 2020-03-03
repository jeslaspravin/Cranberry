#include "VulkanMemoryResources.h"
#include "../../VulkanGraphicsHelper.h"
#include "../VulkanDevice.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanBufferResource, VK_OBJECT_TYPE_BUFFER)

void VulkanBufferResource::init()
{
    reinitResources();
}

void VulkanBufferResource::reinitResources()
{
    if (getResourceSize() == 0)
    {
        Logger::error("VulkanBufferResource", "%s() : Invalid resource %s", __func__, bufferName.getChar());
        return;
    }

    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(graphicsInstance);
    VkBuffer nextBuffer = VulkanGraphicsHelper::createBuffer(graphicsInstance,requiredSize(), bufferUsage);

    if (nextBuffer)
    {
        release();
        buffer = nextBuffer;
        graphicsDebugger->markObject(this);
        VulkanGraphicsHelper::allocateBufferResource(graphicsInstance, this, false);
    }
    else
    {
        Logger::error("VulkanBufferResource", "%s() : Failed creating buffer %s", __func__, bufferName.getChar());
    }
}

void VulkanBufferResource::release()
{
    if (buffer)
    {
        IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
        VulkanGraphicsHelper::deallocateBufferResource(graphicsInstance, this);
        VulkanGraphicsHelper::destroyBuffer(graphicsInstance, buffer);
        buffer = nullptr;
    }
}

String VulkanBufferResource::getObjectName() const
{
    return getResourceName();
}

void VulkanBufferResource::setObjectName(const String& name)
{
    bufferName = name;
}

uint64 VulkanBufferResource::getDispatchableHandle() const
{
    return (uint64)buffer;
}

uint64 VulkanBufferResource::requiredSize()
{
    return getResourceSize();
}

bool VulkanBufferResource::canAllocateMemory()
{
    return buffer && requiredSize() > 0;
}

String VulkanBufferResource::getResourceName() const
{
    return bufferName;
}

