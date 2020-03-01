#pragma once
#include "../RenderInterface/GraphicsHelper.h"
#include <vulkan_core.h>

class GenericAppWindow;

class VulkanGraphicsHelper : public GraphicsHelperAPI<VulkanGraphicsHelper>
{
public:

    static VkInstance getInstance(class IGraphicsInstance* graphicsInstance);
    static VkDevice getDevice(const class VulkanDevice* vulkanDevice);
    static const class VulkanDebugGraphics* debugGraphics(class IGraphicsInstance* graphicsInstance);


    static VkSwapchainKHR createSwapchain(class IGraphicsInstance* graphicsInstance, GenericAppWindow* appWindow);
    static void fillSwapchainImages(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain,std::vector<VkImage>* images);
    static void destroySwapchain(class IGraphicsInstance* graphicsInstance,VkSwapchainKHR swapchain);
    static uint32 getNextSwapchainImage(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain,
        SharedPtr<GraphicsSemaphore>* waitOnSemaphore, SharedPtr<GraphicsFence>* waitOnFence = nullptr);
    static void presentImage(class IGraphicsInstance* graphicsInstance, std::vector<GenericWindowCanvas*>* canvases,
        std::vector<uint32>* imageIndex, std::vector<SharedPtr<class GraphicsSemaphore>>* waitOnSemaphores);


    static SharedPtr<class GraphicsSemaphore> createSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName);
    static SharedPtr<class GraphicsTimelineSemaphore> createTimelineSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName);
    static void waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
        std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores,std::vector<uint64>* waitForValues);
    static SharedPtr<class GraphicsFence> createFence(class IGraphicsInstance* graphicsInstance, const char* fenceName);
    static void waitFences(class IGraphicsInstance* graphicsInstance,std::vector<SharedPtr<class GraphicsFence>>* fences,
        bool waitAll);

    static VkBuffer createBuffer(class IGraphicsInstance* graphicsInstance, const uint64& size,
        const VkBufferUsageFlags& usageFlags);
    static void destroyBuffer(class IGraphicsInstance* graphicsInstance, VkBuffer buffer);
    static bool allocateBufferResource(class IGraphicsInstance* graphicsInstance,
        class IVulkanMemoryResources* memoryResource, bool cpuAccessible);
    static void deallocateBufferResource(class IGraphicsInstance* graphicsInstance,
        class IVulkanMemoryResources* memoryResource);
};


namespace GraphicsTypes
{
    typedef GraphicsHelperAPI<VulkanGraphicsHelper> GraphicsHelper;
}
