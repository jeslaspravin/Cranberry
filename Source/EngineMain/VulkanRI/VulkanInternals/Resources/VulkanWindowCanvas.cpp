#include "VulkanWindowCanvas.h"
#include "../../../Core/Platform/GenericAppWindow.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../VulkanFunctions.h"
#include "../../VulkanGraphicsHelper.h"
#include "../../../RenderInterface/Resources/GraphicsSyncResource.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include <assert.h>

DEFINE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas, VK_OBJECT_TYPE_SURFACE_KHR)

void VulkanWindowCanvas::init()
{
	if (!ownerWindow || !ownerWindow->isValidWindow() || !gEngine)
	{
		Logger::error("VkSurfaceKHR", "%s() : Cannot initialize Vulkan windows canvas without valid windows", __func__);
		return;
	}

	BaseType::init();

	Vk::vkCreatePlatformSurfaceKHR.setInstanceWindow(gEngine->getApplicationInstance(), ownerWindow);
	Vk::vkCreatePlatformSurfaceKHR(VulkanGraphicsHelper::getInstance(gEngine->getRenderApi()->getGraphicsInstance()), 
		nullptr, nullptr, &surfacePtr);
	reinitResources();
}

void VulkanWindowCanvas::release()
{
	if (swapchainPtr || swapchainPtr != VK_NULL_HANDLE)
	{
		VulkanGraphicsHelper::destroySwapchain(gEngine->getRenderApi()->getGraphicsInstance(), swapchainPtr);
	}
	swapchainPtr = nullptr;
	for (int32 i = 0; i < swapchainImages.size(); ++i)
	{
		semaphores[i]->release();
		fences[i]->release();
	}
	semaphores.clear();
	fences.clear();
	swapchainImages.clear();

	Vk::vkDestroySurfaceKHR(VulkanGraphicsHelper::getInstance(gEngine->getRenderApi()->getGraphicsInstance()),
		surfacePtr, nullptr);
	surfacePtr = nullptr;
}

uint32 VulkanWindowCanvas::requestNextImage(SharedPtr<GraphicsSemaphore>* waitOnSemaphore,
	SharedPtr<GraphicsFence>* waitOnFence /*= nullptr*/)
{
	currentSyncIdx = ++currentSyncIdx % swapchainImages.size();
	uint32 currentSwapchainIdx = 0;

	if (fences[currentSyncIdx]->isSignaled())
	{
		fences[currentSyncIdx]->resetSignal();
	}
	currentSwapchainIdx = VulkanGraphicsHelper::getNextSwapchainImage(gEngine->getRenderApi()->getGraphicsInstance(),
		swapchainPtr, &semaphores[currentSyncIdx], &fences[currentSyncIdx]);
	if (waitOnSemaphore || waitOnFence)
	{
		if (waitOnFence)
			*waitOnFence = fences[currentSyncIdx];
		if (waitOnSemaphore)
			*waitOnSemaphore = semaphores[currentSyncIdx];
	}
	else
	{
		Logger::warn("VulkanWindowCanvas", "%s() : both waiting semaphore and fence being null causes performance lose", __func__);
		// if both are null then wait here
		fences[currentSyncIdx]->waitForSignal();
	}
	return currentSwapchainIdx;
}

VkImage VulkanWindowCanvas::swapchainImage(uint32 index) const
{
	assert(index >= 0 && index < swapchainImages.size());
	return swapchainImages[index];
}

String VulkanWindowCanvas::getObjectName() const
{
	return getResourceName();
}

void VulkanWindowCanvas::reinitResources()
{
	VkSwapchainKHR nextSwapchain = VulkanGraphicsHelper::createSwapchain(gEngine->getRenderApi()->getGraphicsInstance(),
		ownerWindow);
	IGraphicsInstance* gInstance = gEngine->getRenderApi()->getGraphicsInstance();

	if (!nextSwapchain || nextSwapchain == VK_NULL_HANDLE)
	{
		Logger::error("VulkanWindowCanvas", "%s() : failed creating swap chain for surface", __func__);
		return;
	}

	if (swapchainPtr && swapchainPtr != VK_NULL_HANDLE)
	{
		VulkanGraphicsHelper::destroySwapchain(gInstance, swapchainPtr);
		
		for (int32 i = 0; i < swapchainImages.size(); ++i)
		{
			semaphores[i]->release();
			fences[i]->release();
		}
	}
	swapchainPtr = nextSwapchain;
	VulkanGraphicsHelper::fillSwapchainImages(gInstance, swapchainPtr, &swapchainImages);
	if (swapchainImages.size() > 0)
	{
		semaphores.resize(swapchainImages.size());
		fences.resize(swapchainImages.size());
		for (int32 i = 0; i < swapchainImages.size(); ++i)
		{
			semaphores[i] = GraphicsHelper::createSemaphore(gInstance);
			fences[i] = GraphicsHelper::createFence(gInstance);
		}
	}
}
