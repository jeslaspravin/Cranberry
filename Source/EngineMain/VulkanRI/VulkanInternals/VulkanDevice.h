#pragma once
#include "../../Core/Platform/PlatformTypes.h"
#include "Debugging.h"

#include <vector>
#include <vulkan_core.h>

class String;
class QueueResourceBase;

class VulkanDevice
{

private:
    using QueueResourceBasePtr = QueueResourceBase*;

    friend class VulkanGraphicsHelper;
    friend class VulkanMemoryAllocator;

    friend const std::vector<VulkanDevice::QueueResourceBasePtr>& getAllQueues(const VulkanDevice* device);

private:
    VulkanDebugGraphics graphicsDebug;
    VkDevice logicalDevice = nullptr;

    // Physical Device
    VkPhysicalDevice physicalDevice = nullptr;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceTimelineSemaphorePropertiesKHR timelineSemaphoreProps;
    VkPhysicalDeviceMemoryProperties memoryProperties;

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceFeatures enabledFeatures;
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaphoreFeatures;
    void markEnabledFeatures();
    void markGlobalConstants();
    // Physical Device

    // Queues
    std::vector<VkQueueFamilyProperties> queueFamiliesSupported;
    std::vector<QueueResourceBasePtr> allQueues;
    QueueResourceBasePtr graphicsQueue = nullptr;
    QueueResourceBasePtr computeQueue = nullptr;
    QueueResourceBasePtr transferQueue = nullptr;
    // Only if none of above queues are populated this will be valid
    QueueResourceBasePtr genericQueue = nullptr;
    bool createQueueResources();
    // Queues

    // Extensions and Layers
    std::vector<VkExtensionProperties> availableExtensions;
    std::vector<const char*> registeredExtensions;
    bool collectDeviceExtensions(std::vector<const char*>& extensions) const;
    
    std::vector<VkLayerProperties> availableLayers;
    std::vector<const char*> registeredLayers;
#if _DEBUG
    void collectDeviceLayers(std::vector<const char*>& layers) const;
#endif
    // Extensions and Layers


    void loadDeviceFunctions();

    // Swap chain and surface    
    VkPresentModeKHR globalPresentMode;
    VkSurfaceFormatKHR swapchainFormat{ VkFormat::VK_FORMAT_UNDEFINED };
    uint32 choosenImageCount = 1;
    VkImageUsageFlags swapchainImgUsage;

    int32 compareSurfaceCompatibility(const class GenericWindowCanvas* surfaceCanvas,const VulkanDevice& otherDevice) const;
    int32 compareMemoryCompatibility(const VulkanDevice& otherDevice) const;
    // Swap chain and surface
public:
    VulkanDevice();
    VulkanDevice(VkPhysicalDevice&& device);
    VulkanDevice(const VulkanDevice& otherDevice);
    VulkanDevice(VulkanDevice&& rVulkanDevice);
    void operator=(const VulkanDevice& otherDevice);
    void operator=(VulkanDevice&& rVulkanDevice);

    ~VulkanDevice();


    void createLogicDevice();
    void cacheGlobalSurfaceProperties();
    void freeLogicDevice();

    String getDeviceName() const;
    VkPresentModeKHR getPresentMode() const;
    QueueResourceBasePtr getGraphicsQueue() const;
    QueueResourceBasePtr getComputeQueue() const;
    QueueResourceBasePtr getTransferQueue() const;
    QueueResourceBasePtr getGenericQueue() const;
    const VulkanDebugGraphics* debugGraphics() const { return &graphicsDebug; }

    [[nodiscard]] int32 compare(const VulkanDevice& otherDevice) const;
    [[nodiscard]] bool isValidDevice() const;

    void getMemoryStat(uint64& totalBudget, uint64& usage,uint32 heapIndex);

public:

#define DEVICE_VK_FUNCTIONS(function) PFN_##function function;
#define DEVICE_VK_EXT_FUNCTIONS(function,extension) PFN_##function function;

#include "VulkanFunctionLists.inl"
};

struct VulkanDeviceCompare {
    bool operator()(const VulkanDevice& lhs, const VulkanDevice& rhs) {
        return lhs.compare(rhs)>=0? true:false;
    }
};