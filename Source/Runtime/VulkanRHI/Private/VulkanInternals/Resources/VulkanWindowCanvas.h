/*!
 * \file VulkanWindowCanvas.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Math/CoreMathTypedefs.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "String/String.h"
#include "VulkanInternals/Resources/IVulkanResources.h"
#include "VulkanInternals/VulkanMacros.h"

#include <vector>

struct SwapchainInfo
{
    VkFormat format;
    UInt2 size;
};

class VulkanWindowCanvas final
    : public GenericWindowCanvas
    , public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas, , GenericWindowCanvas, )
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
    VulkanWindowCanvas(GenericAppWindow *window)
        : BaseType(window)
        , surfacePtr(nullptr)
        , swapchainPtr(nullptr)
    {}

    void init() override;
    void reinitResources() override;
    void release() override;

    uint32 requestNextImage(SemaphoreRef *waitOnSemaphore, FenceRef *waitOnFence = nullptr) override;
    EPixelDataFormat::Type windowCanvasFormat() const override;
    int32 imagesCount() const override;

    VkSurfaceKHR surface() const { return surfacePtr; }
    VkSwapchainKHR swapchain() const { return swapchainPtr; }
    VkImage swapchainImage(uint32 index) const;
    VkImageView swapchainImageView(uint32 index) const;

    String getObjectName() const override;
};
