#include "VulkanWindowCanvas.h"
#include "../../../Core/Platform/GenericAppWindow.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../VulkanFunctions.h"
#include "../../VulkanGraphicsHelper.h"
#include "../../../RenderInterface/Resources/GraphicsSyncResource.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include "../Debugging.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas, VK_OBJECT_TYPE_SURFACE_KHR)

void VulkanWindowCanvas::init()
{
    if (!ownerWindow || !ownerWindow->isValidWindow() || !gEngine)
    {
        Logger::error("VkSurfaceKHR", "%s() : Cannot initialize Vulkan windows canvas without valid windows", __func__);
        return;
    }

    BaseType::init();
    IGraphicsInstance* gInstance = gEngine->getRenderApi()->getGraphicsInstance();

    Vk::vkCreatePlatformSurfaceKHR.setInstanceWindow(gEngine->getApplicationInstance(), ownerWindow);
    Vk::vkCreatePlatformSurfaceKHR(VulkanGraphicsHelper::getInstance(gInstance), 
        nullptr, nullptr, &surfacePtr);
    reinitResources();
}

void VulkanWindowCanvas::release()
{
    if (swapchainPtr || swapchainPtr != VK_NULL_HANDLE)
    {
        VulkanGraphicsHelper::destroySwapchain(gEngine->getRenderApi()->getGraphicsInstance(), swapchainPtr);
    }
    swapchainPtr = nullptr;
    for (int32 i = 0; i < swapchainImages.size(); ++i)
    {
        semaphores[i]->release();
        fences[i]->release();
    }
    semaphores.clear();
    fences.clear();
    swapchainImages.clear();

    Vk::vkDestroySurfaceKHR(VulkanGraphicsHelper::getInstance(gEngine->getRenderApi()->getGraphicsInstance()),
        surfacePtr, nullptr);
    surfacePtr = nullptr;
}

uint32 VulkanWindowCanvas::requestNextImage(SharedPtr<GraphicsSemaphore>* waitOnSemaphore,
    SharedPtr<GraphicsFence>* waitOnFence /*= nullptr*/)
{
    currentSyncIdx = ++currentSyncIdx % swapchainImages.size();
    uint32 currentSwapchainIdx = 0;
    
    if (fences[currentSyncIdx]->isSignaled())
    {
        fences[currentSyncIdx]->resetSignal();
    }

    SharedPtr<GraphicsSemaphore>* semaphorePtr = waitOnSemaphore ? &semaphores[currentSyncIdx] : nullptr;
    SharedPtr<GraphicsFence>* fencePtr = waitOnFence || !waitOnSemaphore ? &fences[currentSyncIdx] : nullptr;
    currentSwapchainIdx = VulkanGraphicsHelper::getNextSwapchainImage(gEngine->getRenderApi()->getGraphicsInstance(),
        swapchainPtr, semaphorePtr, fencePtr);

    if (waitOnSemaphore || waitOnFence)
    {
        if (waitOnFence)
            *waitOnFence = fences[currentSyncIdx];
        if (waitOnSemaphore)
            *waitOnSemaphore = semaphores[currentSyncIdx];
    }
    else
    {
        Logger::warn("VulkanWindowCanvas", "%s() : both waiting semaphore and fence being null is source of performance lose/bug", __func__);
        // if both are null then wait here
        fences[currentSyncIdx]->waitForSignal();
    }
    return currentSwapchainIdx;
}

VkImage VulkanWindowCanvas::swapchainImage(uint32 index) const
{
    fatalAssert(index >= 0 && index < swapchainImages.size(),"Invalid swapchain index");
    return swapchainImages[index];
}

VkImageView VulkanWindowCanvas::swapchainImageView(uint32 index) const
{
    fatalAssert(index >= 0 && index < swapchainImages.size(), "Invalid swapchain index");
    return swapchainImageViews[index];
}

String VulkanWindowCanvas::getObjectName() const
{
    return getResourceName();
}

void VulkanWindowCanvas::reinitResources()
{
    VkSwapchainKHR nextSwapchain = VulkanGraphicsHelper::createSwapchain(gEngine->getRenderApi()->getGraphicsInstance(),
        ownerWindow);
    IGraphicsInstance* gInstance = gEngine->getRenderApi()->getGraphicsInstance();

    if (!nextSwapchain || nextSwapchain == VK_NULL_HANDLE)
    {
        Logger::error("VulkanWindowCanvas", "%s() : failed creating swap chain for surface", __func__);
        return;
    }
    const String& windowName = ownerWindow->getWindowName();

    VulkanGraphicsHelper::debugGraphics(gInstance)->markObject((uint64)surfacePtr,
        windowName+"Surface", VK_OBJECT_TYPE_SURFACE_KHR);

    if (swapchainPtr && swapchainPtr != VK_NULL_HANDLE)
    {
        VulkanGraphicsHelper::destroySwapchain(gInstance, swapchainPtr);
        
        for (int32 i = 0; i < swapchainImages.size(); ++i)
        {
            semaphores[i]->release();
            fences[i]->release();
        }
    }
    swapchainPtr = nextSwapchain;
    VulkanGraphicsHelper::debugGraphics(gInstance)->markObject((uint64)swapchainPtr,
        windowName+"Swapchain", VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    VulkanGraphicsHelper::fillSwapchainImages(gInstance, swapchainPtr, &swapchainImages, &swapchainImageViews);
    if (swapchainImages.size() > 0)
    {
        semaphores.resize(swapchainImages.size());
        fences.resize(swapchainImages.size());
        for (int32 i = 0; i < swapchainImages.size(); ++i)
        {
            String indexString = std::to_string(i);
            semaphores[i] = GraphicsHelper::createSemaphore(gInstance,(windowName + "Semaphore" + indexString).c_str());
            fences[i] = GraphicsHelper::createFence(gInstance,(windowName + "Fence" + indexString).c_str());

            VulkanGraphicsHelper::debugGraphics(gInstance)->markObject((uint64)swapchainImages[i],
                (windowName + "Image" + indexString), VK_OBJECT_TYPE_IMAGE);
            VulkanGraphicsHelper::debugGraphics(gInstance)->markObject((uint64)swapchainImageViews[i],
                (windowName + "ImageView" + indexString), VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}
