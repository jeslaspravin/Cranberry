#pragma once


#include "../../../RenderInterface/Resources/GraphicsSyncResource.h"
#include "../../Resources/IVulkanResources.h"
#include "../VulkanMacros.h"
#include "../../../Core/String/String.h"

#include <vulkan_core.h>

class VulkanDevice;

class VulkanSemaphore : public GraphicsSemaphore,public IVulkanResources
{
	DECLARE_VK_GRAPHICS_RESOURCE(VulkanSemaphore,,GraphicsSemaphore,)

private:

	const VulkanDevice* vulkanDevice;
	VkDevice ownerDevice;	
	VulkanSemaphore(){}

public:
	VkSemaphore semaphore;

	VulkanSemaphore(VkDevice device,const VulkanDevice* deviceInstance);

	void waitForSignal() const override;
	bool isSignaled() const override;
	void resetSignal() override;
	void init() override;
	void reinitResources() override;
	void release() override;
};

class VulkanTimelineSemaphore : public GraphicsTimelineSemaphore, public IVulkanResources
{
	DECLARE_VK_GRAPHICS_RESOURCE(VulkanTimelineSemaphore, , GraphicsTimelineSemaphore, )

private:

	const VulkanDevice* vulkanDevice;
	VkDevice ownerDevice;
	VulkanTimelineSemaphore() {}

public:
	VkSemaphore semaphore;

	VulkanTimelineSemaphore(VkDevice device,const VulkanDevice* deviceInstance);

	void init() override;
	void reinitResources() override;
	void release() override;
	void waitForSignal(uint64 value) const override;
	bool isSignaled(uint64 value) const override;
	void resetSignal(uint64 value) override;
	uint64 currentValue() const override;

};

class VulkanFence : public GraphicsFence, public IVulkanResources
{

	DECLARE_VK_GRAPHICS_RESOURCE(VulkanFence, , GraphicsFence, )

private:

	const VulkanDevice* vulkanDevice;
	VkDevice ownerDevice;
	VulkanFence() {}

public:
	VkFence fence;

	VulkanFence(VkDevice device,const VulkanDevice* deviceInstance);

	void waitForSignal() const override;
	bool isSignaled() const override;
	void resetSignal() override;
	void init() override;
	void reinitResources() override;
	void release() override;
};