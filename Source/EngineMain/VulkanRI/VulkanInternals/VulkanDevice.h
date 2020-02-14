#pragma once
#include "../../Core/Platform/PlatformTypes.h"

#include <vector>
#include <vulkan_core.h>
#include "Debugging.h"

class String;
class QueueResourceBase;



class VulkanDevice {

	friend class VulkanGraphicsHelper;

private:
	VulkanDebugGraphics graphicsDebug;
	VkDevice logicalDevice = nullptr;

	// Physical Device
	VkPhysicalDevice physicalDevice = nullptr;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceTimelineSemaphorePropertiesKHR timelineSemaphoreProps;

	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceFeatures enabledFeatures;
	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaphoreFeatures;
	void markEnabledFeatures();
	// Physical Device

	// Queues
	std::vector<VkQueueFamilyProperties> queueFamiliesSupported;
	typedef QueueResourceBase* QueueResourceBasePtr;
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
	VkSurfaceCapabilitiesKHR swapchainCapabilities;
	VkSurfaceFormatKHR swapchainFormat;
	uint32 choosenImageCount = 1;
	VkImageUsageFlags swapchainImgUsage;

	void cacheGlobalSurfaceProperties();
	int32 compareSurfaceCompatibility(const class GenericWindowCanvas* surfaceCanvas,const VulkanDevice& otherDevice) const;
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

	// For time line semaphore.
	uint64 maxAllowedTimelineOffset() const;

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