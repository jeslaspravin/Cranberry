#pragma once
#include "../../Core/Memory/SmartPointers.h"
#include <vulkan_core.h>

class VulkanDevice;
struct VulkanMemoryBlock;

class IVulkanMemoryAllocator
{
protected:
    VulkanDevice* device;
    IVulkanMemoryAllocator(VulkanDevice* vDevice);
public:
    virtual ~IVulkanMemoryAllocator(){}

    static SharedPtr<IVulkanMemoryAllocator> createAllocator(VulkanDevice* vDevice);

    virtual void initAllocator() = 0;
    virtual void destroyAllocator() = 0;

    virtual VulkanMemoryBlock* allocateBuffer(VkBuffer buffer, bool cpuAccessible) = 0;
    virtual VulkanMemoryBlock* allocateImage(VkImage image, bool cpuAccessible) = 0;

    virtual void deallocateBuffer(VkBuffer buffer, VulkanMemoryBlock* block) = 0;
    virtual void deallocateImage(VkImage image, VulkanMemoryBlock* block) = 0;
};

namespace std {
    template<>
    struct default_delete<IVulkanMemoryAllocator>
    {
        void operator()(IVulkanMemoryAllocator* _Ptr) const noexcept {
            _Ptr->destroyAllocator();
            delete _Ptr;
        }
    };
}