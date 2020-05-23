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
#include "VulkanInternals/Resources/VulkanMemoryResources.h"
#include "VulkanInternals/VulkanMemoryAllocator.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "VulkanInternals/Resources/VulkanSampler.h"
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Core/Math/Math.h"

template <EQueueFunction QueueFunction> VulkanQueueResource<QueueFunction>* getQueue(const VulkanDevice* device);

VkInstance VulkanGraphicsHelper::getInstance(class IGraphicsInstance* graphicsInstance)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    return gInstance->vulkanInstance;
}


VkDevice VulkanGraphicsHelper::getDevice(const class VulkanDevice* vulkanDevice)
{
    return vulkanDevice->logicalDevice;
}

const VulkanDebugGraphics* VulkanGraphicsHelper::debugGraphics(class IGraphicsInstance* graphicsInstance)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    return device->debugGraphics();
}

class VulkanDescriptorsSetAllocator* VulkanGraphicsHelper::getDescriptorsSetAllocator(class IGraphicsInstance* graphicsInstance)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    return gInstance->descriptorsSetAllocator.get();
}

VkSwapchainKHR VulkanGraphicsHelper::createSwapchain(class IGraphicsInstance* graphicsInstance, 
    GenericAppWindow* appWindow,struct SwapchainInfo* swapchainInfo)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
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

    VulkanQueueResource<EQueueFunction::Present>* presentQueue = getQueue<EQueueFunction::Present>(device);
    VulkanQueueResource<EQueueFunction::Graphics>* graphicsQueue = getQueue<EQueueFunction::Graphics>(device);

    fatalAssert(presentQueue && graphicsQueue,"presenting queue or graphics queue cannot be null");

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

    /* Updating other necessary values */
    VkExtent2D surfaceSize = device->swapchainCapabilities.currentExtent;
    if (surfaceSize.height == 0xFFFFFFFF
        || surfaceSize.width == 0xFFFFFFFF)
    {
        surfaceSize.height = Math::clamp<uint32>(EngineSettings::screenSize.get().x, device->swapchainCapabilities.minImageExtent.height,
            device->swapchainCapabilities.maxImageExtent.height);
        surfaceSize.width = Math::clamp<uint32>(EngineSettings::screenSize.get().y, device->swapchainCapabilities.minImageExtent.width,
            device->swapchainCapabilities.maxImageExtent.width);
        EngineSettings::screenSize.set(Size2D(surfaceSize.width, surfaceSize.height));
    }
    else
    {
        // surfaceSize will always give window's actual size in machine so setting it back to window class
        appWindow->setWindowSize(surfaceSize.width, surfaceSize.height, false);
    }
    EngineSettings::surfaceSize.set(Size2D(surfaceSize.width, surfaceSize.height));
    swapchainCreateInfo.imageExtent = surfaceSize;

    VkSwapchainKHR swapchain;
    device->vkCreateSwapchainKHR(device->logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);

    if (swapchainInfo)
    {
        swapchainInfo->format = device->swapchainFormat.format;
    }

    return swapchain;
}

void VulkanGraphicsHelper::fillSwapchainImages(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain, std::vector<VkImage>* images, std::vector<VkImageView>* imageViews)
{
    if (!images || !imageViews)
    {
        return;
    }
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    uint32 imageCount;
    device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, nullptr);
    images->resize(imageCount);
    device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, images->data());

    imageViews->resize(imageCount);

    IMAGE_VIEW_CREATE_INFO(imgViewCreateInfo);
    imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imgViewCreateInfo.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        VK_REMAINING_MIP_LEVELS,
        0,
        VK_REMAINING_ARRAY_LAYERS
    };
    imgViewCreateInfo.format = device->swapchainFormat.format;
    for (uint32 i = 0; i < imageCount; ++i)
    {
        imgViewCreateInfo.image = images->at(i);
        (*imageViews)[i] = createImageView(graphicsInstance, imgViewCreateInfo);
    }
}

void VulkanGraphicsHelper::destroySwapchain(class IGraphicsInstance* graphicsInstance, VkSwapchainKHR swapchain)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
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
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
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

    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
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

    VkResult result = device->vkQueuePresentKHR(getQueue<EQueueFunction::Present>(device)
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
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    auto* semaphore = new VulkanSemaphore(device);
    semaphore->setResourceName(semaphoreName);
    semaphore->init();
    return SharedPtr<GraphicsSemaphore>(semaphore);
}

SharedPtr<class GraphicsTimelineSemaphore> VulkanGraphicsHelper::createTimelineSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    auto* tSemaphore = new VulkanTimelineSemaphore(device);
    tSemaphore->setResourceName(semaphoreName);
    tSemaphore->init();
    return SharedPtr<GraphicsTimelineSemaphore>(tSemaphore);
}

void VulkanGraphicsHelper::waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
    std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores, std::vector<uint64>* waitForValues)
{
    fatalAssert(semaphores->size() <= waitForValues->size(),"Cannot wait on semaphores if the wait for values is less than waiting semaphors count");

    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
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

SharedPtr<class GraphicsFence> VulkanGraphicsHelper::createFence(class IGraphicsInstance* graphicsInstance, const char* fenceName, bool bIsSignaled /*= false*/)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    VulkanFence* fence = new VulkanFence(device,bIsSignaled);
    fence->setResourceName(fenceName);
    fence->init();
    return SharedPtr<GraphicsFence>(fence);
}

void VulkanGraphicsHelper::waitFences(class IGraphicsInstance* graphicsInstance, 
    std::vector<SharedPtr<class GraphicsFence>>* fences, bool waitAll)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
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

VkBuffer VulkanGraphicsHelper::createBuffer(class IGraphicsInstance* graphicsInstance, const VkBufferCreateInfo& bufferCreateInfo
    , EPixelDataFormat::Type bufferDataFormat)
{
    VkBuffer buffer = nullptr;

    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;


    VkFormatFeatureFlags requiredFeatures = (bufferCreateInfo.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) > 0 ?
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT : 0;
    requiredFeatures |= (bufferCreateInfo.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) > 0 ?
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT : 0;
    if (requiredFeatures > 0)
    {
        const EPixelDataFormat::ImageFormatInfo* imageFormatInfo = EPixelDataFormat::getFormatInfo(bufferDataFormat);
        if (bufferDataFormat == EPixelDataFormat::Undefined || !imageFormatInfo)
        {
            Logger::error("NewBufferCreation", "%s() : Invalid expected pixel format for buffer", __func__);
            return buffer;
        }

        VkFormatProperties formatProps;
        Vk::vkGetPhysicalDeviceFormatProperties(device->physicalDevice, (VkFormat)imageFormatInfo->format, &formatProps);

        if ((formatProps.bufferFeatures & requiredFeatures) != requiredFeatures)
        {
            Logger::error("NewBufferCreation", "%s() : Required format %s for buffer is not supported by device"
                , __func__, imageFormatInfo->formatName.getChar());
            return buffer;
        }
    }

    if (device->vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        buffer = nullptr;
    }

    return buffer;
}

void VulkanGraphicsHelper::destroyBuffer(class IGraphicsInstance* graphicsInstance, VkBuffer buffer)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    device->vkDestroyBuffer(device->logicalDevice, buffer, nullptr);
}

bool VulkanGraphicsHelper::allocateBufferResource(class IGraphicsInstance* graphicsInstance, 
    class IVulkanMemoryResources* memoryResource, bool cpuAccessible)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    auto* resource = static_cast<VulkanBufferResource*>(memoryResource);
    VulkanMemoryBlock* block = gInstance->memoryAllocator->allocateBuffer(resource->buffer,cpuAccessible);
    if (block)
    {
        memoryResource->setMemoryData(block);
        gInstance->selectedDevice.vkBindBufferMemory(gInstance->selectedDevice.logicalDevice, resource->buffer, 
            memoryResource->getDeviceMemory(), memoryResource->allocationOffset());
        return true;
    }
    return false;
}

void VulkanGraphicsHelper::deallocateBufferResource(class IGraphicsInstance* graphicsInstance,
    class IVulkanMemoryResources* memoryResource)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    auto* resource = static_cast<VulkanBufferResource*>(memoryResource);
    if (memoryResource->getMemoryData())
    {
        gInstance->memoryAllocator->deallocateBuffer(resource->buffer, memoryResource->getMemoryData());
    }
}

void VulkanGraphicsHelper::mapResource(class IGraphicsInstance* graphicsInstance, BufferResource* buffer)
{
    VulkanGraphicsInstance* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    IVulkanMemoryResources* memoryResource = static_cast<VulkanBufferResource*>(buffer);

    if (memoryResource->getMappedMemory() == nullptr)
    {
        gInstance->memoryAllocator->mapBuffer(memoryResource->getMemoryData());
    }
}

void VulkanGraphicsHelper::unmapResource(class IGraphicsInstance* graphicsInstance, BufferResource* buffer)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    IVulkanMemoryResources* memoryResource = static_cast<VulkanBufferResource*>(buffer);

    if (memoryResource->getMappedMemory() != nullptr)
    {
        gInstance->memoryAllocator->unmapBuffer(memoryResource->getMemoryData());
    }
}

VkBufferView VulkanGraphicsHelper::createBufferView(class IGraphicsInstance* graphicsInstance, const VkBufferViewCreateInfo& viewCreateInfo)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    VkBufferView view;
    if (device->vkCreateBufferView(device->logicalDevice, &viewCreateInfo, nullptr, &view) != VK_SUCCESS)
    {
        Logger::error("VulkanGraphicsHelper", "%s() : Buffer view creation failed", __func__);
        view = nullptr;
    }
    return view;
}

void VulkanGraphicsHelper::destroyBufferView(class IGraphicsInstance* graphicsInstance, VkBufferView view)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    device->vkDestroyBufferView(device->logicalDevice, view, nullptr);
}

VkImage VulkanGraphicsHelper::createImage(class IGraphicsInstance* graphicsInstance, VkImageCreateInfo& createInfo
    , VkFormatFeatureFlags& requiredFeatures)
{
    VkImage image = nullptr;

    const VulkanGraphicsInstance* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    
    if(requiredFeatures > 0)
    {
        VkFormatProperties pixelFormatProperties;
        Vk::vkGetPhysicalDeviceFormatProperties(device->physicalDevice, createInfo.format, &pixelFormatProperties);
        VkFormatFeatureFlags availableFeatures = createInfo.tiling == VK_IMAGE_TILING_LINEAR ?
            pixelFormatProperties.linearTilingFeatures : pixelFormatProperties.optimalTilingFeatures;
        if ((availableFeatures & requiredFeatures) != requiredFeatures)
        {
            Logger::error("NewImageCreation", "%s() : Required format for image is not supported by device", __func__);
            return image;
        }
    }

    VkImageFormatProperties imageFormatProperties;
    Vk::vkGetPhysicalDeviceImageFormatProperties(device->physicalDevice, createInfo.format, createInfo.imageType
        , createInfo.tiling, createInfo.usage, createInfo.flags, &imageFormatProperties);
    if (imageFormatProperties.maxExtent.width < createInfo.extent.width || imageFormatProperties.maxExtent.height < createInfo.extent.height
        || imageFormatProperties.maxExtent.depth < createInfo.extent.depth)
    {
        Logger::error("NewImageCreation", "%s() : Image size (%d, %d, %d) is exceeding the maximum size (%d, %d, %d) supported by device"
            , __func__, createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth, imageFormatProperties.maxExtent.width
            , imageFormatProperties.maxExtent.height, imageFormatProperties.maxExtent.depth);
        return image;
    }

    if (createInfo.arrayLayers > imageFormatProperties.maxArrayLayers)
    {
        Logger::warn("NewImageCreation", "%s() : Image layer count %d is exceeding the maximum layer count %d supported by device, using max limit"
            , __func__,createInfo.arrayLayers,imageFormatProperties.maxArrayLayers );
        createInfo.arrayLayers = imageFormatProperties.maxArrayLayers;
    }

    if (createInfo.mipLevels > imageFormatProperties.maxMipLevels)
    {
        Logger::warn("NewImageCreation", "%s() : Image mip levels %d is exceeding the maximum mip levels %d supported by device, using max limit"
            , __func__, createInfo.mipLevels, imageFormatProperties.maxMipLevels);
        createInfo.mipLevels = imageFormatProperties.maxMipLevels;
    }

    if (device->vkCreateImage(device->logicalDevice, &createInfo, nullptr, &image) != VK_SUCCESS)
    {
        image = nullptr;
    }
    return image;
}

void VulkanGraphicsHelper::destroyImage(class IGraphicsInstance* graphicsInstance, VkImage image)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    device->vkDestroyImage(device->logicalDevice, image, nullptr);
}

bool VulkanGraphicsHelper::allocateImageResource(class IGraphicsInstance* graphicsInstance
    , class IVulkanMemoryResources* memoryResource, bool cpuAccessible)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    auto* resource = static_cast<VulkanImageResource*>(memoryResource);
    VulkanMemoryBlock* block = gInstance->memoryAllocator->allocateImage(resource->image, cpuAccessible, 
        !resource->isStagingResource());// Every image apart from staging image are optimal

    if (block)
    {
        memoryResource->setMemoryData(block);
        gInstance->selectedDevice.vkBindImageMemory(gInstance->selectedDevice.logicalDevice, resource->image,
            memoryResource->getDeviceMemory(), memoryResource->allocationOffset());
        return true;
    }
    return false;
}

void VulkanGraphicsHelper::deallocateImageResource(class IGraphicsInstance* graphicsInstance
    , class IVulkanMemoryResources* memoryResource)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    auto* resource = static_cast<VulkanImageResource*>(memoryResource);
    if (memoryResource->getMemoryData())
    {
        gInstance->memoryAllocator->deallocateImage(resource->image, memoryResource->getMemoryData(),
            !resource->isStagingResource());// Every image apart from staging image are optimal
    }
}

void VulkanGraphicsHelper::mapResource(class IGraphicsInstance* graphicsInstance, ImageResource* image)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    IVulkanMemoryResources* memoryResource = static_cast<VulkanImageResource*>(image);

    if (memoryResource->getMappedMemory() == nullptr && image->isStagingResource())
    {
        gInstance->memoryAllocator->mapImage(memoryResource->getMemoryData());
    }
}

void VulkanGraphicsHelper::unmapResource(class IGraphicsInstance* graphicsInstance, ImageResource* image)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    IVulkanMemoryResources* memoryResource = static_cast<VulkanImageResource*>(image);

    if (memoryResource->getMappedMemory() != nullptr && image->isStagingResource())
    {
        gInstance->memoryAllocator->unmapImage(memoryResource->getMemoryData());
    }
}

VkImageView VulkanGraphicsHelper::createImageView(class IGraphicsInstance* graphicsInstance, const VkImageViewCreateInfo& viewCreateInfo)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    VkImageView view;
    if (device->vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view) != VK_SUCCESS)
    {
        Logger::error("VulkanGraphicsHelper", "%s() : Image view creation failed", __func__);
        view = nullptr;
    }
    return view;
}

void VulkanGraphicsHelper::destroyImageView(class IGraphicsInstance* graphicsInstance, VkImageView view)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    device->vkDestroyImageView(device->logicalDevice, view, nullptr);
}

SharedPtr<class SamplerInterface> VulkanGraphicsHelper::createSampler(class IGraphicsInstance* graphicsInstance,
    const char* name, ESamplerTilingMode::Type samplerTiling, ESamplerFiltering::Type samplerFiltering, float poorMipLod)
{
    auto* gInstance = static_cast<VulkanGraphicsInstance*>(graphicsInstance);
    VulkanSampler* sampler = new VulkanSampler(&gInstance->selectedDevice, samplerTiling, samplerFiltering, poorMipLod);
    sampler->setResourceName(name);
    sampler->init();

    return SharedPtr<SamplerInterface>(sampler);
}

void* VulkanGraphicsHelper::borrowMappedPtr(class IGraphicsInstance* graphicsInstance, class GraphicsResource* resource)
{
    if (resource->getType()->isChildOf(ImageResource::staticType()))
    {
        auto* imgRes = static_cast<VulkanImageResource*>(resource);
        mapResource(graphicsInstance, imgRes);
        return imgRes->getMappedMemory();
    }
    else if(resource->getType()->isChildOf(BufferResource::staticType()))
    {
        auto* bufferRes = static_cast<VulkanBufferResource*>(resource);
        mapResource(graphicsInstance, bufferRes);
        return bufferRes->getMappedMemory();
    }
    return nullptr;
}

void VulkanGraphicsHelper::returnMappedPtr(class IGraphicsInstance* graphicsInstance, class GraphicsResource* resource)
{
    if (resource->getType()->isChildOf(ImageResource::staticType()))
    {
        unmapResource(graphicsInstance, static_cast<ImageResource*>(resource));
    }
    else if (resource->getType()->isChildOf(BufferResource::staticType()))
    {
        unmapResource(graphicsInstance, static_cast<BufferResource*>(resource));
    }
}

VkShaderModule VulkanGraphicsHelper::createShaderModule(class IGraphicsInstance* graphicsInstance, const uint8* code, uint32 size)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    SHADER_MODULE_CREATE_INFO(createInfo);
    createInfo.codeSize = size;
    createInfo.pCode = reinterpret_cast<const uint32*>(code);

    VkShaderModule shaderModule;
    if (device->vkCreateShaderModule(device->logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        Logger::error("VulkanGraphicsHelper", "%s() : failure in creating shader module[Shader size : %d]", __func__, size);
        shaderModule = nullptr;
    }
    return shaderModule;
}

void VulkanGraphicsHelper::destroyShaderModule(class IGraphicsInstance* graphicsInstance, VkShaderModule shaderModule)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    device->vkDestroyShaderModule(device->logicalDevice, shaderModule, nullptr);
}

void VulkanGraphicsHelper::destroyRenderPass(class IGraphicsInstance* graphicsInstance, VkRenderPass renderPass)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    device->vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);
}

void VulkanGraphicsHelper::createFramebuffer(class IGraphicsInstance* graphicsInstance, VkFramebufferCreateInfo& fbCreateInfo, VkFramebuffer* framebuffer)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice; 
    if (device->vkCreateFramebuffer(device->logicalDevice, &fbCreateInfo, nullptr, framebuffer) != VK_SUCCESS)
    {
        Logger::error("VulkanGraphicsHelper", "%s() : Failed creating framebuffer", __func__);
        (*framebuffer) = nullptr;
    }
}

void VulkanGraphicsHelper::destroyFramebuffer(class IGraphicsInstance* graphicsInstance, VkFramebuffer framebuffer)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;
    device->vkDestroyFramebuffer(device->logicalDevice, framebuffer, nullptr);
}
