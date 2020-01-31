#pragma once
#include "../../Core/Platform/PlatformTypes.h"

#include <vector>
#include "vulkan_core.h"

class String;
class QueueResourceBase;



class VulkanDevice {

private:
	VkDevice logicalDevice = nullptr;

	VkPhysicalDevice physicalDevice = nullptr;
	VkPhysicalDeviceProperties properties;

	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceFeatures enabledFeatures;
	void featuresToEnable(VkPhysicalDeviceFeatures& enableFeatures);

	std::vector<VkQueueFamilyProperties> queueFamiliesSupported;
	typedef QueueResourceBase* QueueResourceBasePtr;
	std::vector<QueueResourceBasePtr> allQueues;
	QueueResourceBasePtr graphicsQueue = nullptr;
	QueueResourceBasePtr computeQueue = nullptr;
	QueueResourceBasePtr transferQueue = nullptr;
	QueueResourceBasePtr genericQueue = nullptr;// Only if none of above queues are populated this will be valid
	bool createQueueResources();

	std::vector<VkExtensionProperties> availableExtensions;
	std::vector<const char*> registeredExtensions;
	bool collectDeviceExtensions(std::vector<const char*>& extensions) const;

#if _DEBUG
	std::vector<VkLayerProperties> availableLayers;
	std::vector<const char*> registeredLayers;
	void collectDeviceLayers(std::vector<const char*>& layers) const;
#endif

	void loadDeviceFunctions();

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

	String getDeviceName();

	[[nodiscard]] int32 compare(const VulkanDevice& otherDevice) const;
	[[nodiscard]] bool isValidDevice() const;

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