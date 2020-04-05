#pragma once
#include "../RenderInterface/GraphicsHelper.h"
#include "../RenderInterface/Resources/MemoryResources.h"

#include <vulkan_core.h>

class GenericAppWindow;

class VulkanGraphicsHelper : public GraphicsHelperAPI<VulkanGraphicsHelper>
{
public:

    static VkInstance getInstance(class IGraphicsInstance* graphicsInstance);
    static VkDevice getDevice(const class VulkanDevice* vulkanDevice);
    static const class VulkanDebugGraphics* debugGraphics(class IGraphicsInstance* graphicsInstance);

    // Only in experimental branch
    static class VulkanDevice* getVulkanDevice(class IGraphicsInstance* graphicsInstance);
    static std::vector<class QueueResourceBase*>* getVDAllQueues(VulkanDevice* device);

    static VkSwapchainKHR createSwapchain(class IGraphicsInstance* graphicsInstance, GenericAppWindow* appWindow);
    static void fillSwapchainImages(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain,std::vector<VkImage>* images, std::vector<VkImageView>* imageViews);
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

    static VkBuffer createBuffer(class IGraphicsInstance* graphicsInstance, const VkBufferCreateInfo& bufferCreateInfo
        , EPixelDataFormat::Type bufferDataFormat);
    static void destroyBuffer(class IGraphicsInstance* graphicsInstance, VkBuffer buffer);
    static bool allocateBufferResource(class IGraphicsInstance* graphicsInstance,
        class IVulkanMemoryResources* memoryResource, bool cpuAccessible);
    static void deallocateBufferResource(class IGraphicsInstance* graphicsInstance,
        class IVulkanMemoryResources* memoryResource);
    static VkBufferView createBufferView(class IGraphicsInstance* graphicsInstance, const VkBufferViewCreateInfo& viewCreateInfo);
    static void destroyBufferView(class IGraphicsInstance* graphicsInstance, VkBufferView view);

    static VkImage createImage(class IGraphicsInstance* graphicsInstance, VkImageCreateInfo& createInfo
        , VkFormatFeatureFlags& requiredFeatures);
    static void destroyImage(class IGraphicsInstance* graphicsInstance, VkImage image);
    static bool allocateImageResource(class IGraphicsInstance* graphicsInstance,
        class IVulkanMemoryResources* memoryResource, bool cpuAccessible);
    static void deallocateImageResource(class IGraphicsInstance* graphicsInstance,
        class IVulkanMemoryResources* memoryResource);
    static VkImageView createImageView(class IGraphicsInstance* graphicsInstance, const VkImageViewCreateInfo& viewCreateInfo);
    static void destroyImageView(class IGraphicsInstance* graphicsInstance, VkImageView view);

    static SharedPtr<class SamplerInterface> createSampler(class IGraphicsInstance* graphicsInstance, const char* name,
        ESamplerTilingMode::Type samplerTiling, ESamplerFiltering::Type samplerFiltering, float poorMipLod);
};


namespace GraphicsTypes
{
    typedef GraphicsHelperAPI<VulkanGraphicsHelper> GraphicsHelper;
}
