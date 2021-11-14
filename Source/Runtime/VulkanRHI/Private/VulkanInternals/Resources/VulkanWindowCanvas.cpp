#include "VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "VulkanInternals/Debugging.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "GenericAppWindow.h"
#include "Logger/Logger.h"
#include "VulkanGraphicsHelper.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanRHIModule.h"
#include "IApplicationModule.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanWindowCanvas, VK_OBJECT_TYPE_SURFACE_KHR)

void VulkanWindowCanvas::init()
{
    BaseType::init();
    if (!ownerWindow || !ownerWindow->isValidWindow())
    {
        Logger::error("VkSurfaceKHR", "%s() : Cannot initialize Vulkan windows canvas without valid windows", __func__);
        return;
    }

    BaseType::init();
    IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    Vk::vkCreatePlatformSurfaceKHR.setInstanceWindow(IApplicationModule::get()->getApplication(), ownerWindow);
    Vk::vkCreatePlatformSurfaceKHR(VulkanGraphicsHelper::getInstance(graphicsInstance), 
        nullptr, nullptr, &surfacePtr);
    reinitResources();
}

void VulkanWindowCanvas::reinitResources()
{
    BaseType::reinitResources();
    SwapchainInfo currentInfo = swapchainInfo;
    IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI* graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();

    VkSwapchainKHR nextSwapchain = VulkanGraphicsHelper::createSwapchain(graphicsInstance, this, &swapchainInfo);

    if (!nextSwapchain || nextSwapchain == VK_NULL_HANDLE)
    {
        Logger::error("VulkanWindowCanvas", "%s() : failed creating swap chain for surface", __func__);
        return;
    }
    const String& windowName = ownerWindow->getWindowName();

    VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject((uint64)surfacePtr,
        windowName + "Surface", VK_OBJECT_TYPE_SURFACE_KHR);

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
    VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject((uint64)swapchainPtr,
        windowName + "Swapchain", VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    VulkanGraphicsHelper::fillSwapchainImages(graphicsInstance, swapchainPtr, &swapchainImages, &swapchainImageViews);
    if (swapchainImages.size() > 0)
    {
        semaphores.resize(swapchainImages.size());
        fences.resize(swapchainImages.size());
        for (int32 i = 0; i < swapchainImages.size(); ++i)
        {
            String indexString = std::to_string(i);
            semaphores[i] = graphicsHelper->createSemaphore(graphicsInstance, (windowName + "Semaphore" + indexString).c_str());
            semaphores[i]->init();
            fences[i] = graphicsHelper->createFence(graphicsInstance, (windowName + "Fence" + indexString).c_str());
            fences[i]->init();

            VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject((uint64)swapchainImages[i],
                (windowName + "Image" + indexString), VK_OBJECT_TYPE_IMAGE);
            VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject((uint64)swapchainImageViews[i],
                (windowName + "ImageView" + indexString), VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }

    ownerWindow->setWindowSize(swapchainInfo.size.x, swapchainInfo.size.y, false);
    currentImageSize = swapchainInfo.size;
}

void VulkanWindowCanvas::release()
{
    BaseType::release();
    IGraphicsInstance* gInstance = IVulkanRHIModule::get()->getGraphicsInstance();
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

    Vk::vkDestroySurfaceKHR(VulkanGraphicsHelper::getInstance(gInstance),surfacePtr, nullptr);
    surfacePtr = nullptr;
}

uint32 VulkanWindowCanvas::requestNextImage(SemaphoreRef* waitOnSemaphore, FenceRef* waitOnFence/*= nullptr*/)
{
    currentSyncIdx = ++currentSyncIdx % swapchainImages.size();
    uint32 nextSwapchainIdx = 0;
    
    if (fences[currentSyncIdx]->isSignaled())
    {
        fences[currentSyncIdx]->resetSignal();
    }

    SemaphoreRef* semaphorePtr = (waitOnSemaphore ? &semaphores[currentSyncIdx] : nullptr);
    FenceRef* fencePtr = ((waitOnFence || !waitOnSemaphore) ? &fences[currentSyncIdx] : nullptr);

    nextSwapchainIdx = VulkanGraphicsHelper::getNextSwapchainImage(IVulkanRHIModule::get()->getGraphicsInstance(),
        swapchainPtr, semaphorePtr, fencePtr);

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
        Logger::warn("VulkanWindowCanvas", "%s() : both waiting semaphore and fence being null is source of performance lose/bug", __func__);
        // if both are null then wait here
        fences[currentSyncIdx]->waitForSignal();
        currentFence = fences[currentSyncIdx];
        currentSemaphore.reset();
    }
    currentSwapchainIdx = nextSwapchainIdx;
    return nextSwapchainIdx;
}

EPixelDataFormat::Type VulkanWindowCanvas::windowCanvasFormat() const 
{
    return EngineToVulkanAPI::vulkanToEngineDataFormat(swapchainInfo.format);
}

int32 VulkanWindowCanvas::imagesCount() const 
{
    return (int32)swapchainImages.size();
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
