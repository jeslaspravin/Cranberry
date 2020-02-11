#pragma once
#include "../../../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../VulkanMacros.h"
#include "../../Resources/IVulkanResources.h"
#include "../../../Core/String/String.h"

class VulkanWindowCanvas : public GenericWindowCanvas,public IVulkanResources
{
	DECLARE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas,,GenericWindowCanvas,)
private:

	VkSurfaceKHR surfacePtr;
	VkSwapchainKHR swapchainPtr;
	std::vector<VkImage> swapchainImages;

public:
	void init() override;
	void reinitResources() override;

	void release() override;

	VkSurfaceKHR surface() const { return surfacePtr; }
	VkSwapchainKHR swapchain() const { return swapchainPtr; }

};
