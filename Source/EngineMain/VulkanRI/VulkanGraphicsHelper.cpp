#include <assert.h>
#include <glm/common.hpp>
#include <vector>

#include "VulkanGraphicsHelper.h"
#include "VulkanGraphicsInstance.h"
#include "VulkanInternals/VulkanDevice.h"
#include "../Core/Platform/GenericAppWindow.h"
#include "VulkanInternals/VulkanMacros.h"
#include "../Core/Engine/GameEngine.h"
#include "VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "VulkanInternals/Resources/VulkanQueueResource.h"
#include "VulkanInternals/Resources/VulkanSyncResource.h"


VkInstance VulkanGraphicsHelper::getInstance(class IGraphicsInstance* graphicsInstance)
{
    VulkanGraphicsInstance* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    return gInstance->vulkanInstance;
}


VkDevice VulkanGraphicsHelper::getDevice(const class VulkanDevice* vulkanDevice)
{
    return vulkanDevice->logicalDevice;
}

const VulkanDebugGraphics* VulkanGraphicsHelper::debugGraphics(class IGraphicsInstance* graphicsInstance)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    return device->debugGraphics();
}

class VulkanDevice* VulkanGraphicsHelper::getVulkanDevice(class IGraphicsInstance* graphicsInstance)
{
    VulkanGraphicsInstance* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    return &gInstance->selectedDevice;
}

std::vector<class QueueResourceBase*>* VulkanGraphicsHelper::getVDAllQueues(VulkanDevice* device)
{
    return &device->allQueues;
}

template <EQueueFunction QueueFunction>
VulkanQueueResource<QueueFunction>* getQueue(const std::vector<QueueResourceBase*>& allQueues, const VulkanDevice* device);

VkSwapchainKHR VulkanGraphicsHelper::createSwapchain(class IGraphicsInstance* graphicsInstance, 
    GenericAppWindow* appWindow)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    if (!device->isValidDevice())
    {
        Logger::error("VulkanSwapchain", "%s() : Cannot access resources of invalid device", __func__);
        return nullptr;
    }

    CREATE_SWAPCHAIN_INFO(swapchainCreateInfo);
    swapchainCreateInfo.surface = static_cast<VulkanWindowCanvas*>(gEngine->getApplicationInstance()
        ->appWindowManager.getWindowCanvas(appWindow))->surface();
    swapchainCreateInfo.minImageCount = device->choosenImageCount;
    swapchainCreateInfo.imageFormat = device->swapchainFormat.format;
    swapchainCreateInfo.imageColorSpace = device->swapchainFormat.colorSpace;
    swapchainCreateInfo.presentMode = device->globalPresentMode;
    swapchainCreateInfo.oldSwapchain = static_cast<VulkanWindowCanvas*>(gEngine->getApplicationInstance()
        ->appWindowManager.getWindowCanvas(appWindow))->swapchain();
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.clipped = VK_FALSE;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.imageUsage = device->swapchainImgUsage;

    VulkanQueueResource<EQueueFunction::Present>* presentQueue = getQueue<EQueueFunction::Present>(device->allQueues,device);
    VulkanQueueResource<EQueueFunction::Graphics>* graphicsQueue = getQueue<EQueueFunction::Graphics>(device->allQueues, device);

    assert(presentQueue && graphicsQueue);

    if (presentQueue->queueFamilyIndex() == graphicsQueue->queueFamilyIndex())
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        std::vector<uint32> queueFamilyIndices = { graphicsQueue->queueFamilyIndex(),presentQueue->queueFamilyIndex() };
        swapchainCreateInfo.queueFamilyIndexCount = (uint32_t)queueFamilyIndices.size();
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }

    VkExtent2D surfaceSize = device->swapchainCapabilities.currentExtent;
    if (surfaceSize.height == 0xFFFFFFFF
        || surfaceSize.width == 0xFFFFFFFF)
    {
        appWindow->windowSize(surfaceSize.width, surfaceSize.height);
        surfaceSize.height = glm::clamp<uint32>(surfaceSize.height, device->swapchainCapabilities.minImageExtent.height,
            device->swapchainCapabilities.maxImageExtent.height);
        surfaceSize.width = glm::clamp<uint32>(surfaceSize.width, device->swapchainCapabilities.minImageExtent.width,
            device->swapchainCapabilities.maxImageExtent.width);
    }
    else
    {
        appWindow->setWindowSize(surfaceSize.width, surfaceSize.height, false);
    }
    swapchainCreateInfo.imageExtent = surfaceSize;

    VkSwapchainKHR swapchain;
    device->vkCreateSwapchainKHR(device->logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);
    return swapchain;
}

void VulkanGraphicsHelper::fillSwapchainImages(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain, std::vector<VkImage>* images)
{
    if (!images)
    {
        return;
    }
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    uint32 imageCount;
    device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, nullptr);
    images->resize(imageCount);
    device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, images->data());
}

void VulkanGraphicsHelper::destroySwapchain(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    if (!device->isValidDevice())
    {
        Logger::error("VulkanSwapchain", "%s() : Cannot access resources of invalid device", __func__);
        return;
    }

    device->vkDestroySwapchainKHR(device->logicalDevice, swapchain,nullptr);
}

uint32 VulkanGraphicsHelper::getNextSwapchainImage(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain, 
    SharedPtr<GraphicsSemaphore>* waitOnSemaphore, SharedPtr<GraphicsFence>* waitOnFence /*= nullptr*/)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    uint32 imageIndex;
    VkSemaphore semaphore = (waitOnSemaphore && *waitOnSemaphore) ?
        static_cast<VulkanSemaphore*>(waitOnSemaphore->get())->semaphore : VK_NULL_HANDLE;
    VkFence fence = (waitOnFence && *waitOnFence)?
        static_cast<VulkanFence*>(waitOnFence->get())->fence : VK_NULL_HANDLE;
    VkResult result = device->vkAcquireNextImageKHR(device->logicalDevice, swapchain, 2000000000, semaphore, fence, &imageIndex);

    if (result == VK_TIMEOUT)
    {
        Logger::error("VulkanSwapchain", "%s() : Timed out waiting to acquire next swapchain image", __func__);
        return -1;
    }
    else if (result == VK_NOT_READY)
    {
        Logger::error("VulkanSwapchain", "%s() : swapchain is not suitable for use", __func__);
        return -1;
    }
    return imageIndex;
}

void VulkanGraphicsHelper::presentImage(class IGraphicsInstance* graphicsInstance, std::vector<GenericWindowCanvas*>* canvases
    , std::vector<uint32>* imageIndex, std::vector<SharedPtr<class GraphicsSemaphore>>* waitOnSemaphores)
{
    if (!canvases || !imageIndex || canvases->size() != imageIndex->size())
        return;

    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    std::vector<VkSwapchainKHR> swapchains(canvases->size());
    std::vector<VkResult> results(canvases->size());
    std::vector<VkSemaphore> semaphores(waitOnSemaphores ? waitOnSemaphores->size() : 0);

    for (int32 i = 0; i < swapchains.size(); ++i)
    {
        swapchains[i] = static_cast<VulkanWindowCanvas*>((*canvases)[i])->swapchain();
    }

    for (int32 i = 0; i < semaphores.size(); ++i)
    {
        semaphores[i] = static_cast<VulkanSemaphore*>((*waitOnSemaphores)[i].get())->semaphore;
    }
    PRESENT_INFO(presentInfo);
    presentInfo.pImageIndices = imageIndex->data();
    presentInfo.pSwapchains = swapchains.data();
    presentInfo.swapchainCount = (uint32)swapchains.size();
    presentInfo.pResults = results.data();
    presentInfo.pWaitSemaphores = semaphores.size() > 0?semaphores.data():nullptr;
    presentInfo.waitSemaphoreCount = (uint32)semaphores.size();

    VkResult result = device->vkQueuePresentKHR(getQueue<EQueueFunction::Present>(device->allQueues, device)
        ->getQueueOfPriority<EQueuePriority::SuperHigh>(), &presentInfo);

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        Logger::error("VulkanPresenting", "%s() : Failed to present images", __func__);
    }
    else
    {
        for (int32 i = 0; i < results.size(); ++i)
        {
            if (results[i] != VK_SUCCESS && results[i] != VK_SUBOPTIMAL_KHR)
            {
                Logger::error("VulkanPresenting", "%s() : Failed presenting for window %s", __func__,
                    (*canvases)[i]->getResourceName().getChar());
            }
        }
    }
}

SharedPtr<class GraphicsSemaphore> VulkanGraphicsHelper::createSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    VulkanSemaphore* semaphore = new VulkanSemaphore(device);
    semaphore->setObjectName(semaphoreName);
    semaphore->init();
    return SharedPtr<GraphicsSemaphore>(semaphore);
}

SharedPtr<class GraphicsTimelineSemaphore> VulkanGraphicsHelper::createTimelineSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    VulkanTimelineSemaphore* tSemaphore = new VulkanTimelineSemaphore(device);
    tSemaphore->setObjectName(semaphoreName);
    tSemaphore->init();
    return SharedPtr<GraphicsTimelineSemaphore>(tSemaphore);
}

void VulkanGraphicsHelper::waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
    std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores, std::vector<uint64>* waitForValues)
{
    assert(semaphores->size() == waitForValues->size());

    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    std::vector<VkSemaphore> deviceSemaphores;
    deviceSemaphores.reserve(semaphores->size());
    for (const SharedPtr<GraphicsTimelineSemaphore>& semaphore : *semaphores)
    {
        deviceSemaphores.push_back(static_cast<const VulkanTimelineSemaphore*>(semaphore.get())->semaphore);
    }

    SEMAPHORE_WAIT_INFO(waitInfo);
    waitInfo.pSemaphores = deviceSemaphores.data();
    waitInfo.semaphoreCount = (uint32)deviceSemaphores.size();
    waitInfo.pValues = waitForValues->data();

    device->vkWaitSemaphoresKHR(device->logicalDevice, &waitInfo, 2000000000/*2 Seconds*/);
}

SharedPtr<class GraphicsFence> VulkanGraphicsHelper::createFence(class IGraphicsInstance* graphicsInstance, const char* fenceName)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    VulkanFence* fence = new VulkanFence(device);
    fence->setObjectName(fenceName);
    fence->init();
    return SharedPtr<GraphicsFence>(fence);
}

void VulkanGraphicsHelper::waitFences(class IGraphicsInstance* graphicsInstance, 
    std::vector<SharedPtr<class GraphicsFence>>* fences, bool waitAll)
{
    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    std::vector<VkFence> deviceFences;
    deviceFences.reserve(fences->size());
    for (const SharedPtr<GraphicsFence>& fence : *fences)
    {
        deviceFences.push_back(static_cast<const VulkanFence*>(fence.get())->fence);
    }

    device->vkWaitForFences(device->logicalDevice, (uint32)deviceFences.size(), deviceFences.data(),waitAll? VK_TRUE:VK_FALSE
        , 2000000000/*2 Seconds*/);
}
