#pragma once
#include "../../Core/Memory/SmartPointers.h"

class VulkanDevice;

class IVulkanMemoryAllocator
{
protected:
    VulkanDevice* device;
    IVulkanMemoryAllocator(VulkanDevice* vDevice);
public:
    static SharedPtr<IVulkanMemoryAllocator> createAllocator(VulkanDevice* vDevice);

    virtual void initAllocator() = 0;
    virtual void destroyAllocator() = 0;
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