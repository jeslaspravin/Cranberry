#include "VulkanWindowCanvas.h"
#include "../../../Core/Platform/GenericAppWindow.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../VulkanFunctions.h"
#include "../../VulkanGraphicsHelper.h"

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

	Vk::vkDestroySurfaceKHR(VulkanGraphicsHelper::getInstance(gEngine->getRenderApi()->getGraphicsInstance()),
		surfacePtr, nullptr);
	surfacePtr = nullptr;
}

void VulkanWindowCanvas::reinitResources()
{
	VkSwapchainKHR nextSwapchain = VulkanGraphicsHelper::createSwapchain(gEngine->getRenderApi()->getGraphicsInstance(),
		ownerWindow);

	if (!nextSwapchain || nextSwapchain == VK_NULL_HANDLE)
	{
		Logger::error("VulkanWindowCanvas", "%s() : failed creating swap chain for surface", __func__);
		return;
	}

	if (swapchainPtr && swapchainPtr != VK_NULL_HANDLE)
	{
		VulkanGraphicsHelper::destroySwapchain(gEngine->getRenderApi()->getGraphicsInstance(), swapchainPtr);
	}
	swapchainPtr = nextSwapchain;
	VulkanGraphicsHelper::fillSwapchainImages(gEngine->getRenderApi()->getGraphicsInstance(), swapchainPtr, &swapchainImages);
}
