#pragma once
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "VulkanInternals/VulkanMacros.h"
#include "VulkanInternals/Resources/IVulkanResources.h"
#include "String/String.h"
#include "Math/CoreMathTypedefs.h"

#include <vector>

struct SwapchainInfo
{
    VkFormat format;
    Size2D size;
};

class VulkanWindowCanvas final : public GenericWindowCanvas, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas,,GenericWindowCanvas,)
private:
    VkSurfaceKHR surfacePtr;
    VkSwapchainKHR swapchainPtr;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<SemaphoreRef> semaphores;
    std::vector<FenceRef> fences;

    SemaphoreRef currentSemaphore;
    FenceRef currentFence;
    
    SwapchainInfo swapchainInfo;
    int32 currentSyncIdx = -1;
private:
    VulkanWindowCanvas() = default;
public:
    VulkanWindowCanvas(GenericAppWindow* window)
        : BaseType(window)
        , surfacePtr(nullptr)
        , swapchainPtr(nullptr)
    {}

    void init() override;
    void reinitResources() override;
    void release() override;

    uint32 requestNextImage(SemaphoreRef* waitOnSemaphore, FenceRef* waitOnFence = nullptr) override;
    EPixelDataFormat::Type windowCanvasFormat() const override;
    int32 imagesCount() const override;

    VkSurfaceKHR surface() const { return surfacePtr; }
    VkSwapchainKHR swapchain() const { return swapchainPtr; }
    VkImage swapchainImage(uint32 index) const;
    VkImageView swapchainImageView(uint32 index) const;

    String getObjectName() const override;

};
