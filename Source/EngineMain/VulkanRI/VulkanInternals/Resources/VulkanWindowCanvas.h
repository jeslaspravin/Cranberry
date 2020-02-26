#pragma once
#include "../../../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../VulkanMacros.h"
#include "../../Resources/IVulkanResources.h"
#include "../../../Core/String/String.h"

#include <vector>

class VulkanWindowCanvas : public GenericWindowCanvas,public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas,,GenericWindowCanvas,)
private:

    VkSurfaceKHR surfacePtr;
    VkSwapchainKHR swapchainPtr;
    std::vector<VkImage> swapchainImages;
    std::vector<SharedPtr<GraphicsSemaphore>> semaphores;
    std::vector<SharedPtr<GraphicsFence>> fences;
    
    int32 currentSyncIdx = -1;
public:
    void init() override;
    void reinitResources() override;
    void release() override;
    uint32 requestNextImage(SharedPtr<GraphicsSemaphore>* waitOnSemaphore ,SharedPtr<GraphicsFence>* waitOnFence = nullptr) override;

    VkSurfaceKHR surface() const { return surfacePtr; }
    VkSwapchainKHR swapchain() const { return swapchainPtr; }
    VkImage swapchainImage(uint32 index) const;

    String getObjectName() const override;

};
