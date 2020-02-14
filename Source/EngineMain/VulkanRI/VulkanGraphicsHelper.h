#pragma once
#include "../RenderInterface/GraphicsHelper.h"
#include <vulkan_core.h>

class GenericAppWindow;

class VulkanGraphicsHelper : public GraphicsHelperAPI<VulkanGraphicsHelper>
{
public:

	static VkInstance getInstance(class IGraphicsInstance* graphicsInstance);

	static VkSwapchainKHR createSwapchain(class IGraphicsInstance* graphicsInstance, GenericAppWindow* appWindow);
	static void fillSwapchainImages(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain,std::vector<VkImage>* images);
	static void destroySwapchain(class IGraphicsInstance* graphicsInstance,VkSwapchainKHR swapchain);
	static uint32 getNextSwapchainImage(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain,
		SharedPtr<GraphicsSemaphore>* waitOnSemaphore, SharedPtr<GraphicsFence>* waitOnFence = nullptr);
	static void presentImage(class IGraphicsInstance* graphicsInstance, std::vector<GenericWindowCanvas*>* canvases,
		std::vector<uint32>* imageIndex, std::vector<SharedPtr<class GraphicsSemaphore>>* waitOnSemaphores);

	static SharedPtr<class GraphicsSemaphore> createSemaphore(class IGraphicsInstance* graphicsInstance);

	static SharedPtr<class GraphicsTimelineSemaphore> createTimelineSemaphore(class IGraphicsInstance* graphicsInstance);
	static void waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
		std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores,std::vector<uint64>* waitForValues);

	static SharedPtr<class GraphicsFence> createFence(class IGraphicsInstance* graphicsInstance);
	static void waitFences(class IGraphicsInstance* graphicsInstance,
		std::vector<SharedPtr<class GraphicsFence>>* fences,
		bool waitAll);
};


namespace GraphicsTypes
{
	typedef GraphicsHelperAPI<VulkanGraphicsHelper> GraphicsHelper;
}
