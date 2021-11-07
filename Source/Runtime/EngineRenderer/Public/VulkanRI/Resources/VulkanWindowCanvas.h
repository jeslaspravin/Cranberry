#pragma once
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "VulkanRI/VulkanInternals/VulkanMacros.h"
#include "VulkanRI/Resources/IVulkanResources.h"
#include "String/String.h"
#include "Math/CoreMathTypedefs.h"

#include <vector>

struct SwapchainInfo
{
    VkFormat format;
    Size2D size;
};

class ENGINERENDERER_EXPORT VulkanWindowCanvas final : public GenericWindowCanvas, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas,,GenericWindowCanvas,)
private:
    VkSurfaceKHR surfacePtr;
    VkSwapchainKHR swapchainPtr;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<SharedPtr<GraphicsSemaphore>> semaphores;
    std::vector<SharedPtr<GraphicsFence>> fences;

    WeakPtr<GraphicsSemaphore> currentSemaphore;
    WeakPtr<GraphicsFence> currentFence;
    
    SwapchainInfo swapchainInfo;
    int32 currentSyncIdx = -1;
public:
    void init() override;
    void reinitResources() override;
    void release() override;

    uint32 requestNextImage(SharedPtr<GraphicsSemaphore>* waitOnSemaphore ,SharedPtr<GraphicsFence>* waitOnFence = nullptr) override;
    EPixelDataFormat::Type windowCanvasFormat() const override;
    int32 imagesCount() const override;

    VkSurfaceKHR surface() const { return surfacePtr; }
    VkSwapchainKHR swapchain() const { return swapchainPtr; }
    VkImage swapchainImage(uint32 index) const;
    VkImageView swapchainImageView(uint32 index) const;

    String getObjectName() const override;

};
