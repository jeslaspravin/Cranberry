/*!
 * \file VulkanWindowCanvas.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "GenericAppWindow.h"
#include "IApplicationModule.h"
#include "Logger/Logger.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/Debugging.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "VulkanRHIModule.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas, VK_OBJECT_TYPE_SURFACE_KHR)

void VulkanWindowCanvas::init()
{
    BaseType::init();
    if (!ownerWindow || !ownerWindow->isValidWindow())
    {
        LOG_ERROR("VkSurfaceKHR", "Cannot initialize Vulkan windows canvas without valid windows");
        return;
    }

    BaseType::init();
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    Vk::vkCreatePlatformSurfaceKHR.setInstanceWindow(IApplicationModule::get()->getApplication(), ownerWindow);
    Vk::vkCreatePlatformSurfaceKHR(VulkanGraphicsHelper::getInstance(graphicsInstance), nullptr, nullptr, &surfacePtr);
    reinitResources();
}

void VulkanWindowCanvas::reinitResources()
{
    BaseType::reinitResources();
    SwapchainInfo currentInfo = swapchainInfo;
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();

    VkSwapchainKHR nextSwapchain = VulkanGraphicsHelper::createSwapchain(graphicsInstance, this, &swapchainInfo);

    if (!nextSwapchain || nextSwapchain == VK_NULL_HANDLE)
    {
        LOG_ERROR("VulkanWindowCanvas", "failed creating swap chain for surface");
        return;
    }
    const String &windowName = ownerWindow->getWindowName();

    VulkanGraphicsHelper::debugGraphics(graphicsInstance)
        ->markObject((uint64)surfacePtr, windowName + TCHAR("Surface"), VK_OBJECT_TYPE_SURFACE_KHR);

    if (swapchainPtr && swapchainPtr != VK_NULL_HANDLE)
    {
        VulkanGraphicsHelper::destroySwapchain(graphicsInstance, swapchainPtr);

        for (int32 i = 0; i < swapchainImages.size(); ++i)
        {
            semaphores[i]->release();
            fences[i]->release();
            VulkanGraphicsHelper::destroyImageView(graphicsInstance, swapchainImageViews[i]);
        }
    }
    swapchainPtr = nextSwapchain;
    VulkanGraphicsHelper::debugGraphics(graphicsInstance)
        ->markObject((uint64)swapchainPtr, windowName + TCHAR("Swapchain"), VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    VulkanGraphicsHelper::fillSwapchainImages(graphicsInstance, swapchainPtr, &swapchainImages, &swapchainImageViews);
    if (swapchainImages.size() > 0)
    {
        semaphores.resize(swapchainImages.size());
        fences.resize(swapchainImages.size());
        for (int32 i = 0; i < swapchainImages.size(); ++i)
        {
            String indexString = String::toString(i);
            semaphores[i] = graphicsHelper->createSemaphore(graphicsInstance, (windowName + TCHAR("Semaphore") + indexString).c_str());
            semaphores[i]->init();
            fences[i] = graphicsHelper->createFence(graphicsInstance, (windowName + TCHAR("Fence") + indexString).c_str());
            fences[i]->init();

            VulkanGraphicsHelper::debugGraphics(graphicsInstance)
                ->markObject((uint64)swapchainImages[i], (windowName + TCHAR("Image") + indexString), VK_OBJECT_TYPE_IMAGE);
            VulkanGraphicsHelper::debugGraphics(graphicsInstance)
                ->markObject((uint64)swapchainImageViews[i], (windowName + TCHAR("ImageView") + indexString), VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }

    ownerWindow->setWindowSize(swapchainInfo.size.x, swapchainInfo.size.y);
    currentImageSize = swapchainInfo.size;
}

void VulkanWindowCanvas::release()
{
    BaseType::release();
    IGraphicsInstance *gInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    for (int32 i = 0; i < swapchainImages.size(); ++i)
    {
        semaphores[i]->release();
        fences[i]->release();
        VulkanGraphicsHelper::destroyImageView(gInstance, swapchainImageViews[i]);
    }
    semaphores.clear();
    fences.clear();
    swapchainImages.clear();

    if (swapchainPtr || swapchainPtr != VK_NULL_HANDLE)
    {
        VulkanGraphicsHelper::destroySwapchain(gInstance, swapchainPtr);
    }
    swapchainPtr = nullptr;

    Vk::vkDestroySurfaceKHR(VulkanGraphicsHelper::getInstance(gInstance), surfacePtr, nullptr);
    surfacePtr = nullptr;
}

uint32 VulkanWindowCanvas::requestNextImage(SemaphoreRef *waitOnSemaphore, FenceRef *waitOnFence /*= nullptr*/)
{
    currentSyncIdx = ++currentSyncIdx % swapchainImages.size();

    if (fences[currentSyncIdx]->isSignaled())
    {
        fences[currentSyncIdx]->resetSignal();
    }

    SemaphoreRef *semaphorePtr = (waitOnSemaphore ? &semaphores[currentSyncIdx] : nullptr);
    FenceRef *fencePtr = ((waitOnFence || !waitOnSemaphore) ? &fences[currentSyncIdx] : nullptr);

    int32 nextSwapchainIdx
        = VulkanGraphicsHelper::getNextSwapchainImage(IVulkanRHIModule::get()->getGraphicsInstance(), swapchainPtr, semaphorePtr, fencePtr);
    fatalAssertf(nextSwapchainIdx >= 0, "Acquiring next swapchain image failed!");

    if (waitOnSemaphore || waitOnFence)
    {
        if (waitOnFence)
        {
            *waitOnFence = fences[currentSyncIdx];
            currentFence = fences[currentSyncIdx];
            currentSemaphore.reset();
        }
        if (waitOnSemaphore)
        {
            *waitOnSemaphore = semaphores[currentSyncIdx];
            currentSemaphore = semaphores[currentSyncIdx];
            currentFence.reset();
        }
    }
    else
    {
        LOG_WARN("VulkanWindowCanvas", "both waiting semaphore and fence being null is source of performance lose/bug");
        // if both are null then wait here
        fences[currentSyncIdx]->waitForSignal();
        currentFence = fences[currentSyncIdx];
        currentSemaphore.reset();
    }
    currentSwapchainIdx = uint32(nextSwapchainIdx);
    return currentSwapchainIdx;
}

EPixelDataFormat::Type VulkanWindowCanvas::windowCanvasFormat() const
{
    return EngineToVulkanAPI::vulkanToEngineDataFormat(swapchainInfo.format);
}

int32 VulkanWindowCanvas::imagesCount() const { return (int32)swapchainImages.size(); }

VkImage VulkanWindowCanvas::swapchainImage(uint32 index) const
{
    fatalAssertf(index >= 0 && index < swapchainImages.size(), "Invalid swapchain index");
    return swapchainImages[index];
}

VkImageView VulkanWindowCanvas::swapchainImageView(uint32 index) const
{
    fatalAssertf(index >= 0 && index < swapchainImages.size(), "Invalid swapchain index");
    return swapchainImageViews[index];
}

String VulkanWindowCanvas::getObjectName() const { return getResourceName(); }
