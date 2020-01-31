#include "VulkanDevice.h"
#include "../../Core/Logger/Logger.h"
#include "VulkanFunctions.h"
#include "../../Core/String/String.h"
#include "VulkanMacros.h"
#include "Resources/VulkanQueueResource.h"

#include <sstream>
#include <assert.h>
#include <set>

void VulkanDevice::featuresToEnable(VkPhysicalDeviceFeatures& enableFeatures)
{
	// TODO(Jeslas) : Check and enable necessary features.
}

bool VulkanDevice::createQueueResources()
{
	graphicsQueue = new VulkanQueueResource<EQueueFunction::Graphics>(queueFamiliesSupported);
	if (((VulkanQueueResource<EQueueFunction::Graphics>*)graphicsQueue)->isValidQueue())
	{
		allQueues.push_back(graphicsQueue);
	}
	else
	{
		delete graphicsQueue;
		graphicsQueue = nullptr;
	}

	computeQueue = new VulkanQueueResource<EQueueFunction::Compute>(queueFamiliesSupported);
	if (((VulkanQueueResource<EQueueFunction::Compute>*)computeQueue)->isValidQueue())
	{
		allQueues.push_back(computeQueue);
	}
	else
	{
		delete computeQueue;
		computeQueue = nullptr;
	}

	transferQueue = new VulkanQueueResource<EQueueFunction::Transfer>(queueFamiliesSupported);
	if (((VulkanQueueResource<EQueueFunction::Transfer>*)transferQueue)->isValidQueue())
	{
		allQueues.push_back(transferQueue);
	}
	else
	{
		delete transferQueue;
		transferQueue = nullptr;
	}

	if (allQueues.size() != 3)
	{
		genericQueue = new VulkanQueueResource<EQueueFunction::Generic>(queueFamiliesSupported);
		if (((VulkanQueueResource<EQueueFunction::Generic>*)genericQueue)->isValidQueue())
		{
			allQueues.push_back(genericQueue);
		}
		else
		{
			delete genericQueue;
			genericQueue = nullptr;
			Logger::error("VulkanDevice", "%s() : Failed creating necessary queue resources", __func__);
			return false;
		}
	}
	return true;
}

bool VulkanDevice::collectDeviceExtensions(std::vector<const char*>& extensions) const
{
	extensions.clear();

	std::set<const char*> mandatoryExtensions;

#define DEVICE_VK_EXT_FUNCTIONS(function,extension) mandatoryExtensions.insert(extension);

#include "VulkanFunctionLists.inl"

	std::stringstream extensionsStream;
	for (const VkExtensionProperties& extProperty : availableExtensions) {
		extensionsStream << extProperty.extensionName;
	}
	String extensionsString = extensionsStream.str();

	for (const char* mandatoryExt : mandatoryExtensions) {
		if (extensionsString.find(mandatoryExt, 0) != String::npos)
		{
			extensions.push_back(mandatoryExt);
			Logger::debug("VulkanDevice", "%s() : Loading device extension %s", __func__, mandatoryExt);
		}
	}

	if (mandatoryExtensions.size() != extensions.size()) {
		Logger::error("VulkanDevice", "%s() : Missing mandatory extensions", __func__);
		return false;
	}

	return true;
}

#if _DEBUG
void VulkanDevice::collectDeviceLayers(std::vector<const char*>& layers) const
{

}

void VulkanDevice::loadDeviceFunctions()
{
#define DEVICE_VK_FUNCTIONS(function) \
	function = (PFN_##function)Vk::vkGetDeviceProcAddr(logicalDevice,#function); \
	if(function == nullptr) Logger::error("VulkanDevice","%s() : Failed loading function : "#function,__func__);

#define DEVICE_VK_EXT_FUNCTIONS(function,extension) \
	for(const char* ext:registeredExtensions) \
	{\
		if (std::strcmp(ext, extension) == 0)\
		{\
			function = (PFN_##function)Vk::vkGetDeviceProcAddr(logicalDevice,#function); \
			break;\
		}\
	} \
	if (function == nullptr) Logger::error("VulkanDevice", "%s() : Failed loading function : "#function, __func__);

#include "VulkanFunctionLists.inl"
}

#endif

VulkanDevice::VulkanDevice()
{

}

VulkanDevice::VulkanDevice(VkPhysicalDevice&& device)
{
	physicalDevice = device;
	uint32 extCount = 0;
	if (Vk::vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, (uint32_t*)&extCount, nullptr) != VK_SUCCESS)
	{
		Logger::error("VulkanDevice", "%s() : enumerating extensions for device failed", __func__);
		return;
	}
	availableExtensions.resize(extCount);		
	Vk::vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, (uint32_t*)&extCount, availableExtensions.data());

	uint32 layerCount = 0;
	if (Vk::vkEnumerateDeviceLayerProperties(physicalDevice, (uint32_t*)&layerCount, nullptr) != VK_SUCCESS)
	{
		Logger::warn("VulkanDevice", "%s() : enumerating layers for device failed", __func__);
	}
	else
	{
		availableLayers.resize(layerCount);
		Vk::vkEnumerateDeviceLayerProperties(physicalDevice, (uint32_t*)&layerCount, availableLayers.data());
	}

	Vk::vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	Vk::vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	Logger::debug("VulkanDevice", "%s() : Found %d extensions and %d layers in device %s", __func__,
		extCount, layerCount,properties.deviceName);
	Logger::debug("VulkanDevice", "%s() : Device API version %d Driver version %d", __func__, 
		properties.apiVersion, properties.driverVersion);

	uint32 queueCount;
	Vk::vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, (uint32_t*)&queueCount, nullptr);
	queueFamiliesSupported.resize(queueCount);
	Vk::vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, (uint32_t*)&queueCount, queueFamiliesSupported.data());
	Logger::debug("VulkanDevice", "%s() : %s supports %d number of queue families", __func__, properties.deviceName, queueCount);
}

#define MOVE_IMPL() \
availableExtensions = std::move(rVulkanDevice.availableExtensions); \
availableLayers = std::move(rVulkanDevice.availableLayers); \
features = std::move(rVulkanDevice.features); \
properties = std::move(rVulkanDevice.properties); \
queueFamiliesSupported = std::move(rVulkanDevice.queueFamiliesSupported);\
physicalDevice = std::move(rVulkanDevice.physicalDevice); \
graphicsQueue = std::move(rVulkanDevice.graphicsQueue); \
transferQueue = std::move(rVulkanDevice.transferQueue); \
computeQueue = std::move(rVulkanDevice.computeQueue); \
genericQueue = std::move(rVulkanDevice.genericQueue); \
allQueues = std::move(rVulkanDevice.allQueues); \
enabledFeatures = std::move(rVulkanDevice.enabledFeatures); \
logicalDevice = std::move(rVulkanDevice.logicalDevice); \

VulkanDevice::VulkanDevice(VulkanDevice&& rVulkanDevice)
{
	MOVE_IMPL();
}

void VulkanDevice::operator=(VulkanDevice&& rVulkanDevice)
{
	MOVE_IMPL();
}
#undef MOVE_IMPL

#define COPY_IMPL()\
availableExtensions = otherDevice.availableExtensions; \
availableLayers = otherDevice.availableLayers; \
features = otherDevice.features; \
properties = otherDevice.properties; \
queueFamiliesSupported = otherDevice.queueFamiliesSupported; \
physicalDevice = otherDevice.physicalDevice; \
graphicsQueue = otherDevice.graphicsQueue; \
transferQueue = otherDevice.transferQueue; \
computeQueue = otherDevice.computeQueue; \
genericQueue = otherDevice.genericQueue; \
allQueues = otherDevice.allQueues; \
enabledFeatures = std::move(otherDevice.enabledFeatures); \
logicalDevice = std::move(otherDevice.logicalDevice); \

VulkanDevice::VulkanDevice(const VulkanDevice& otherDevice)
{
	COPY_IMPL();
}


void VulkanDevice::operator=(const VulkanDevice& otherDevice)
{
	COPY_IMPL();
}
#undef COPY_IMPL

VulkanDevice::~VulkanDevice()
{
	if (allQueues.size() > 0 || logicalDevice)
	{
		Logger::warn("VulkanDevice", "%s() : Queues & logic devices not cleared");
		freeLogicDevice();
	}
}

void VulkanDevice::createLogicDevice()
{
	Logger::debug("VulkanDevice", "%s() : Creating logical device", __func__);
	assert(createQueueResources());

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(allQueues.size());

	std::set<int32> selectedQueueIndices;// Cannot request create for same queue family twice

	for (int32 queueFamIndex=0;queueFamIndex < allQueues.size();++queueFamIndex)
	{
		CREATE_QUEUE_INFO(queueCreateInfo);
		const QueueResourceBasePtr& queueRes = allQueues[queueFamIndex];

		if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Compute>>())
		{
			static_cast<const VulkanQueueResource<EQueueFunction::Compute>*>(queueRes)
				->getQueueCreateInfo(queueCreateInfo);
		}
		else if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Graphics>>())
		{
			static_cast<const VulkanQueueResource<EQueueFunction::Graphics>*>(queueRes)
				->getQueueCreateInfo(queueCreateInfo);
		}
		else if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Transfer>>())
		{
			static_cast<const VulkanQueueResource<EQueueFunction::Transfer>*>(queueRes)
				->getQueueCreateInfo(queueCreateInfo);
		}
		else if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Generic>>())
		{
			static_cast<const VulkanQueueResource<EQueueFunction::Generic>*>(queueRes)
				->getQueueCreateInfo(queueCreateInfo);
		}

		if (selectedQueueIndices.insert(queueCreateInfo.queueFamilyIndex).second)
		{
			queueCreateInfos.push_back(queueCreateInfo);
		}
	}

#if _DEBUG
	collectDeviceLayers(registeredLayers);
#endif
	assert(collectDeviceExtensions(registeredExtensions));
	featuresToEnable(enabledFeatures);
	
	CREATE_DEVICE_INFO(deviceCreateInfo);
	deviceCreateInfo.enabledExtensionCount = (uint32_t)registeredExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = registeredExtensions.data();
	deviceCreateInfo.enabledLayerCount = (uint32_t)registeredLayers.size();
	deviceCreateInfo.ppEnabledLayerNames = registeredLayers.data();
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	if (Vk::vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS)
	{
		Logger::error("VulkanDevice", "%s() : Failed creating logical device", __func__);
		assert(false);
	}

	loadDeviceFunctions();
}

void VulkanDevice::freeLogicDevice()
{
	Logger::debug("VulkanDevice", "%s() : Freeing logical device", __func__);
	vkDestroyDevice(logicalDevice, nullptr);
	logicalDevice = nullptr;

	for (QueueResourceBasePtr& queueResPtr : allQueues)
	{
		if (queueResPtr)
		{
			queueResPtr->release();
			delete queueResPtr;
		}
	}
	allQueues.clear();
	graphicsQueue = nullptr;
	computeQueue = nullptr;
	transferQueue = nullptr;
	genericQueue = nullptr;
}

String VulkanDevice::getDeviceName()
{
	return properties.deviceName;
}

int32 VulkanDevice::compare(const VulkanDevice& otherDevice) const
{
	if(properties.deviceType != otherDevice.properties.deviceType)
	{
		int32 deviceTypeChoice = 0;

		switch (properties.deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			deviceTypeChoice = -1;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			deviceTypeChoice = 1;
			break;
		default:
			break;
		}

		if (deviceTypeChoice != 0)
		{
			return deviceTypeChoice;
		}

		switch (otherDevice.properties.deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			deviceTypeChoice = 1;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			deviceTypeChoice = -1;
			break;
		default:
			deviceTypeChoice = otherDevice.properties.deviceType - properties.deviceType;
			break;
		}
		return deviceTypeChoice>0?1:-1;
	}

	

	// Todo(Jeslas) : decide between multiple same type  of cards here
	return 0;
}

bool VulkanDevice::isValidDevice() const
{
	return physicalDevice != nullptr && queueFamiliesSupported.size() > 0;
}
