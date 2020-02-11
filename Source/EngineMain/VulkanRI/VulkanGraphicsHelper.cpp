#include <assert.h>
#include <common.hpp>
#include <vector>

#include "VulkanGraphicsHelper.h"
#include "VulkanGraphicsInstance.h"
#include "VulkanInternals/VulkanDevice.h"
#include "../Core/Platform/GenericAppWindow.h"
#include "VulkanInternals/VulkanMacros.h"
#include "../Core/Engine/GameEngine.h"
#include "VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "VulkanInternals/Resources/VulkanQueueResource.h"
#include "VulkanInternals/Resources/VulkanSyncResource.h"


VkInstance VulkanGraphicsHelper::getInstance(class IGraphicsInstance* graphicsInstance)
{
	VulkanGraphicsInstance* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
	return gInstance->vulkanInstance;
}


template <EQueueFunction QueueFunction>
VulkanQueueResource<QueueFunction>* getQueue(const std::vector<QueueResourceBase*>& allQueues, const VulkanDevice* device);

VkSwapchainKHR VulkanGraphicsHelper::createSwapchain(class IGraphicsInstance* graphicsInstance, 
	GenericAppWindow* appWindow)
{
	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	if (!device->isValidDevice())
	{
		Logger::error("VulkanSwapchain", "%s() : Cannot access resources of invalid device", __func__);
		return nullptr;
	}

	CREATE_SWAPCHAIN_INFO(swapchainCreateInfo);
	swapchainCreateInfo.surface = static_cast<VulkanWindowCanvas*>(gEngine->getApplicationInstance()
		->appWindowManager.getWindowCanvas(appWindow))->surface();
	swapchainCreateInfo.minImageCount = device->choosenImageCount;
	swapchainCreateInfo.imageFormat = device->swapchainFormat.format;
	swapchainCreateInfo.imageColorSpace = device->swapchainFormat.colorSpace;
	swapchainCreateInfo.presentMode = device->globalPresentMode;
	swapchainCreateInfo.oldSwapchain = static_cast<VulkanWindowCanvas*>(gEngine->getApplicationInstance()
		->appWindowManager.getWindowCanvas(appWindow))->swapchain();
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.clipped = VK_FALSE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.imageUsage = device->swapchainImgUsage;

	VulkanQueueResource<EQueueFunction::Present>* presentQueue = getQueue<EQueueFunction::Present>(device->allQueues,device);
	VulkanQueueResource<EQueueFunction::Graphics>* graphicsQueue = getQueue<EQueueFunction::Graphics>(device->allQueues, device);

	assert(presentQueue && graphicsQueue);

	if (presentQueue->queueFamilyIndex() == graphicsQueue->queueFamilyIndex())
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		std::vector<uint32> queueFamilyIndices = { graphicsQueue->queueFamilyIndex(),presentQueue->queueFamilyIndex() };
		swapchainCreateInfo.queueFamilyIndexCount = (uint32_t)queueFamilyIndices.size();
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}

	VkExtent2D surfaceSize = device->swapchainCapabilities.currentExtent;
	if (surfaceSize.height == 0xFFFFFFFF
		|| surfaceSize.width == 0xFFFFFFFF)
	{
		appWindow->windowSize(surfaceSize.width, surfaceSize.height);
		surfaceSize.height = glm::clamp<uint32>(surfaceSize.height, device->swapchainCapabilities.minImageExtent.height,
			device->swapchainCapabilities.maxImageExtent.height);
		surfaceSize.width = glm::clamp<uint32>(surfaceSize.width, device->swapchainCapabilities.minImageExtent.width,
			device->swapchainCapabilities.maxImageExtent.width);
	}
	else
	{
		appWindow->setWindowSize(surfaceSize.width, surfaceSize.height, false);
	}
	swapchainCreateInfo.imageExtent = surfaceSize;

	VkSwapchainKHR swapchain;
	device->vkCreateSwapchainKHR(device->logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);
	return swapchain;
}

void VulkanGraphicsHelper::fillSwapchainImages(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain, std::vector<VkImage>* images)
{
	if (!images)
	{
		return;
	}
	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	uint32 imageCount;
	device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, nullptr);
	images->resize(imageCount);
	device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, images->data());
}

void VulkanGraphicsHelper::destroySwapchain(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain)
{
	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	if (!device->isValidDevice())
	{
		Logger::error("VulkanSwapchain", "%s() : Cannot access resources of invalid device", __func__);
		return;
	}

	device->vkDestroySwapchainKHR(device->logicalDevice, swapchain,nullptr);
}

SharedPtr<class GraphicsSemaphore> VulkanGraphicsHelper::createSemaphore(class IGraphicsInstance* graphicsInstance)
{
	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	GraphicsSemaphore* semaphore = new VulkanSemaphore(device->logicalDevice, device);
	semaphore->init();
	return SharedPtr<GraphicsSemaphore>(semaphore);
}

SharedPtr<class GraphicsTimelineSemaphore> VulkanGraphicsHelper::createTimelineSemaphore(class IGraphicsInstance* graphicsInstance)
{
	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	GraphicsTimelineSemaphore* tSemaphore = new VulkanTimelineSemaphore(device->logicalDevice, device);
	tSemaphore->init();
	return SharedPtr<GraphicsTimelineSemaphore>(tSemaphore);
}

void VulkanGraphicsHelper::waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
	std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores, std::vector<uint64>* waitForValues)
{
	assert(semaphores->size() == waitForValues->size());

	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	std::vector<VkSemaphore> deviceSemaphores;
	deviceSemaphores.reserve(semaphores->size());
	for (const SharedPtr<GraphicsTimelineSemaphore>& semaphore : *semaphores)
	{
		deviceSemaphores.push_back(static_cast<const VulkanTimelineSemaphore*>(semaphore.get())->semaphore);
	}

	SEMAPHORE_WAIT_INFO(waitInfo);
	waitInfo.pSemaphores = deviceSemaphores.data();
	waitInfo.semaphoreCount = (uint32)deviceSemaphores.size();
	waitInfo.pValues = waitForValues->data();

	device->vkWaitSemaphoresKHR(device->logicalDevice, &waitInfo, 2000000/*2 Seconds*/);
}

SharedPtr<class GraphicsFence> VulkanGraphicsHelper::createFence(class IGraphicsInstance* graphicsInstance)
{
	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	GraphicsFence* fence = new VulkanFence(device->logicalDevice, device);
	fence->init();
	return SharedPtr<GraphicsFence>(fence);
}

void VulkanGraphicsHelper::waitFences(class IGraphicsInstance* graphicsInstance, 
	std::vector<SharedPtr<class GraphicsFence>>* fences, bool waitAll)
{
	const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
	const VulkanDevice* device = &gInstance->selectedDevice;

	std::vector<VkFence> deviceFences;
	deviceFences.reserve(fences->size());
	for (const SharedPtr<GraphicsFence>& fence : *fences)
	{
		deviceFences.push_back(static_cast<const VulkanFence*>(fence.get())->fence);
	}

	device->vkWaitForFences(device->logicalDevice, (uint32)deviceFences.size(), deviceFences.data(),waitAll? VK_TRUE:VK_FALSE
		, 2000000/*2 Seconds*/);
}
