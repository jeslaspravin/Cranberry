/*!
 * \file VulkanDevice.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/CoreTypes.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "VulkanInternals/Debugging.h"

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
    VkPhysicalDeviceTimelineSemaphoreProperties timelineSemaphoreProps;
    VkPhysicalDeviceDescriptorIndexingProperties descIndexingProps;
    // Zero initialization required here, Since values will be overridden by vkGetPhysicalDeviceMemoryProperties which is external code
    VkPhysicalDeviceMemoryProperties memoryProperties{};

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceDescriptorIndexingFeatures descIndexingFeatures;
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures;
    // KHRs
    VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features;
    // Enabled features will only be partially initialized manually, So zero initialize it
    VkPhysicalDeviceFeatures enabledFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures enabledDescIndexingFeatures{};

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
#if DEV_BUILD
    void collectDeviceLayers(std::vector<const char*>& layers) const;
#endif
    // Extensions and Layers


    void loadDeviceFunctions();

    // Swap chain and surface, Cached    
    std::vector<uint32> presentQueues;// Cached only upto queue is created
    VkPresentModeKHR globalPresentMode;
    VkSurfaceFormatKHR swapchainFormat{ VkFormat::VK_FORMAT_UNDEFINED };
    uint32 choosenImageCount = 1;
    VkImageUsageFlags swapchainImgUsage;

    int32 compareSurfaceCompatibility(const WindowCanvasRef& windowCanvas,const VulkanDevice& otherDevice) const;
    int32 compareMemoryCompatibility(const VulkanDevice& otherDevice) const;
    // Swap chain and surface
public:
    VulkanDevice() = default;
    VulkanDevice(VkPhysicalDevice&& device);
    VulkanDevice(const VulkanDevice& otherDevice);
    VulkanDevice(VulkanDevice&& rVulkanDevice);
    void operator=(const VulkanDevice& otherDevice);
    void operator=(VulkanDevice&& rVulkanDevice);

    ~VulkanDevice();


    void createLogicDevice();
    void cacheGlobalSurfaceProperties(const WindowCanvasRef& windowCanvas);
    void freeLogicDevice();

    String getDeviceName() const;
    VkPresentModeKHR getPresentMode() const;
    QueueResourceBasePtr getGraphicsQueue() const;
    QueueResourceBasePtr getComputeQueue() const;
    QueueResourceBasePtr getTransferQueue() const;
    QueueResourceBasePtr getGenericQueue() const;
    const VulkanDebugGraphics* debugGraphics() const { return &graphicsDebug; }

    NODISCARD int32 compare(const VulkanDevice& otherDevice, const WindowCanvasRef& windowCanvas) const;
    NODISCARD bool isValidDevice() const;
    NODISCARD bool isLogicalDeviceCreated() const;

    void getMemoryStat(uint64& totalBudget, uint64& usage,uint32 heapIndex);

public:

#define DEVICE_VK_FUNCTIONS(function) PFN_##function function;
#define DEVICE_VK_EXT_FUNCTIONS(function,extension) PFN_##function function;

#include "VulkanFunctionLists.inl"
};

struct VulkanDeviceCompare
{
    WindowCanvasRef windowCanvas;

    bool operator()(const VulkanDevice& lhs, const VulkanDevice& rhs) {
        return lhs.compare(rhs, windowCanvas)>=0? true:false;
    }
};