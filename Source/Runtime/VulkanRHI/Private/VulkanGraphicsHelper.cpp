/*!
 * \file VulkanGraphicsHelper.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

// Including on top to avoid redefinition warning on vector and set
#include <set>
#include <vector>

#include "ApplicationSettings.h"
#include "Profiler/ProgramProfiler.hpp"
#include "Math/Math.h"
#include "RenderInterface/Resources/QueueResource.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanGraphicsInstance.h"
#include "VulkanInternals/Rendering/VulkanRenderingContexts.h"
#include "VulkanInternals/Resources/VulkanBufferResources.h"
#include "VulkanInternals/Resources/VulkanImageResources.h"
#include "VulkanInternals/Resources/VulkanPipelines.h"
#include "VulkanInternals/Resources/VulkanQueueResource.h"
#include "VulkanInternals/Resources/VulkanSampler.h"
#include "VulkanInternals/Resources/VulkanShaderResources.h"
#include "VulkanInternals/Resources/VulkanSyncResource.h"
#include "VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "VulkanInternals/ShaderCore/VulkanShaderParamResources.h"
#include "VulkanInternals/VulkanDescriptorAllocator.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "VulkanInternals/VulkanMacros.h"
#include "VulkanInternals/VulkanMemoryAllocator.h"

#if EXPERIMENTAL
// Only in experimental branch
VulkanDevice *VulkanGraphicsHelper::getVulkanDevice(IGraphicsInstance *graphicsInstance)
{
    VulkanGraphicsInstance *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    return &gInstance->selectedDevice;
}

#endif

template <EQueueFunction QueueFunction>
VulkanQueueResource<QueueFunction> *getQueue(const VulkanDevice *device);

VkInstance VulkanGraphicsHelper::getInstance(IGraphicsInstance *graphicsInstance)
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    return gInstance->vulkanInstance;
}

VkDevice VulkanGraphicsHelper::getDevice(const VulkanDevice *vulkanDevice) { return vulkanDevice->logicalDevice; }

const VulkanDebugGraphics *VulkanGraphicsHelper::debugGraphics(IGraphicsInstance *graphicsInstance)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    return device->debugGraphics();
}

VulkanDescriptorsSetAllocator *VulkanGraphicsHelper::getDescriptorsSetAllocator(IGraphicsInstance *graphicsInstance)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    return gInstance->descriptorsSetAllocator.get();
}

VkSwapchainKHR VulkanGraphicsHelper::createSwapchain(
    IGraphicsInstance *graphicsInstance, const GenericWindowCanvas *windowCanvas, struct SwapchainInfo *swapchainInfo
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    if (!device->isValidDevice())
    {
        LOG_ERROR("VulkanSwapchain", "Cannot access resources of invalid device");
        return nullptr;
    }

    if (device->swapchainFormat.format == VkFormat::VK_FORMAT_UNDEFINED)
    {
        LOG_ERROR("VulkanSwapchain", "Surface properties are invalid");
        return nullptr;
    }

    CREATE_SWAPCHAIN_INFO(swapchainCreateInfo);
    swapchainCreateInfo.surface = static_cast<const VulkanWindowCanvas *>(windowCanvas)->surface();
    swapchainCreateInfo.minImageCount = device->choosenImageCount;
    swapchainCreateInfo.imageFormat = device->swapchainFormat.format;
    swapchainCreateInfo.imageColorSpace = device->swapchainFormat.colorSpace;
    swapchainCreateInfo.presentMode = device->globalPresentMode;
    swapchainCreateInfo.oldSwapchain = static_cast<const VulkanWindowCanvas *>(windowCanvas)->swapchain();
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.clipped = VK_FALSE;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.imageUsage = device->swapchainImgUsage;

    VulkanQueueResource<EQueueFunction::Present> *presentQueue = getQueue<EQueueFunction::Present>(device);
    VulkanQueueResource<EQueueFunction::Graphics> *graphicsQueue = getQueue<EQueueFunction::Graphics>(device);

    fatalAssertf(presentQueue && graphicsQueue, "presenting queue or graphics queue cannot be null");

    if (presentQueue->queueFamilyIndex() == graphicsQueue->queueFamilyIndex())
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;

        // Below check is to avoid VK validation error, Cached surface properties works fine in single gpu environment
        VkBool32 queueSupported;
        Vk::vkGetPhysicalDeviceSurfaceSupportKHR(
            device->physicalDevice, presentQueue->queueFamilyIndex(), swapchainCreateInfo.surface, &queueSupported
        );
        fatalAssertf(queueSupported == VK_TRUE, "Window surface created in unsupported device(Multiple GPU is not supported)");
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        std::vector<uint32> queueFamilyIndices = { graphicsQueue->queueFamilyIndex(), presentQueue->queueFamilyIndex() };
        swapchainCreateInfo.queueFamilyIndexCount = (uint32_t)queueFamilyIndices.size();
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

        // Below check is to avoid VK validation error, Cached surface properties works fine in single gpu environment
        VkBool32 presentSupported, graphicsSupported;
        Vk::vkGetPhysicalDeviceSurfaceSupportKHR(
            device->physicalDevice, presentQueue->queueFamilyIndex(), swapchainCreateInfo.surface, &presentSupported
        );
        Vk::vkGetPhysicalDeviceSurfaceSupportKHR(
            device->physicalDevice, graphicsQueue->queueFamilyIndex(), swapchainCreateInfo.surface, &graphicsSupported
        );
        fatalAssertf(
            presentSupported == VK_TRUE && graphicsSupported == VK_TRUE,
            "Window surface created in unsupported device(Multiple GPU is not supported)"
        );
    }

    /* Updating other necessary window unique values */
    VkSurfaceCapabilitiesKHR swapchainCapabilities;
    Vk::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice, swapchainCreateInfo.surface, &swapchainCapabilities);
    VkExtent2D surfaceSize = swapchainCapabilities.currentExtent;
    if (surfaceSize.height == 0xFFFFFFFF || surfaceSize.width == 0xFFFFFFFF)
    {
        surfaceSize.height = Math::clamp<uint32>(
            ApplicationSettings::screenSize.get().x, swapchainCapabilities.minImageExtent.height, swapchainCapabilities.maxImageExtent.height
        );
        surfaceSize.width = Math::clamp<uint32>(
            ApplicationSettings::screenSize.get().y, swapchainCapabilities.minImageExtent.width, swapchainCapabilities.maxImageExtent.width
        );
        ApplicationSettings::screenSize.set(Size2D(surfaceSize.width, surfaceSize.height));
    }
    swapchainCreateInfo.imageExtent = surfaceSize;
    VkSwapchainKHR swapchain;
    device->vkCreateSwapchainKHR(device->logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);

    if (swapchainInfo)
    {
        swapchainInfo->format = device->swapchainFormat.format;
        swapchainInfo->size = Size2D(surfaceSize.width, surfaceSize.height);
    }

    return swapchain;
}

void VulkanGraphicsHelper::fillSwapchainImages(
    IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain, std::vector<VkImage> *images, std::vector<VkImageView> *imageViews
)
{
    if (!images || !imageViews)
    {
        return;
    }
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    uint32 imageCount;
    device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, nullptr);
    images->resize(imageCount);
    device->vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, images->data());

    imageViews->resize(imageCount);

    IMAGE_VIEW_CREATE_INFO(imgViewCreateInfo);
    imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imgViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
    imgViewCreateInfo.format = device->swapchainFormat.format;
    for (uint32 i = 0; i < imageCount; ++i)
    {
        imgViewCreateInfo.image = images->at(i);
        (*imageViews)[i] = createImageView(graphicsInstance, imgViewCreateInfo);
    }
}

void VulkanGraphicsHelper::destroySwapchain(IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    if (!device->isValidDevice())
    {
        LOG_ERROR("VulkanSwapchain", "Cannot access resources of invalid device");
        return;
    }

    device->vkDestroySwapchainKHR(device->logicalDevice, swapchain, nullptr);
}

int32 VulkanGraphicsHelper::getNextSwapchainImage(
    IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain, SemaphoreRef *waitOnSemaphore, FenceRef *waitOnFence /*= nullptr */
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    uint32 imageIndex;
    VkSemaphore semaphore = (waitOnSemaphore && waitOnSemaphore->isValid())
                                ? static_cast<VulkanSemaphore *>(waitOnSemaphore->reference())->semaphore
                                : VK_NULL_HANDLE;
    VkFence fence = (waitOnFence && waitOnFence->isValid()) ? static_cast<VulkanFence *>(waitOnFence->reference())->fence : VK_NULL_HANDLE;
    VkResult result = device->vkAcquireNextImageKHR(device->logicalDevice, swapchain, 2000000000, semaphore, fence, &imageIndex);

    if (result == VK_TIMEOUT)
    {
        LOG_ERROR("VulkanSwapchain", "Timed out waiting to acquire next swapchain image");
        return -1;
    }
    else if (result == VK_NOT_READY)
    {
        LOG_ERROR("VulkanSwapchain", "swapchain is not suitable for use");
        return -1;
    }
    return imageIndex;
}

void VulkanGraphicsHelper::presentImage(
    IGraphicsInstance *graphicsInstance, ArrayView<const WindowCanvasRef> canvases, ArrayView<const uint32> imageIndex,
    ArrayView<const SemaphoreRef> waitOnSemaphores
)
{
    if (canvases.empty() || imageIndex.empty() || canvases.size() != imageIndex.size())
    {
        return;
    }

    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    std::vector<VkSwapchainKHR> swapchains(canvases.size());
    std::vector<VkResult> results(canvases.size());
    std::vector<VkSemaphore> semaphores(waitOnSemaphores.size());

    for (int32 i = 0; i < swapchains.size(); ++i)
    {
        swapchains[i] = canvases[i].reference<VulkanWindowCanvas>()->swapchain();
    }

    for (int32 i = 0; i < semaphores.size(); ++i)
    {
        semaphores[i] = waitOnSemaphores[i].reference<VulkanSemaphore>()->semaphore;
    }
    PRESENT_INFO(presentInfo);
    presentInfo.pImageIndices = imageIndex.data();
    presentInfo.pSwapchains = swapchains.data();
    presentInfo.swapchainCount = (uint32)swapchains.size();
    presentInfo.pResults = results.data();
    presentInfo.pWaitSemaphores = semaphores.size() > 0 ? semaphores.data() : nullptr;
    presentInfo.waitSemaphoreCount = (uint32)semaphores.size();

    VkResult result = VK_ERROR_UNKNOWN;
    {
        CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("PresentASwapchain"));
        result = device->vkQueuePresentKHR(
            getQueue<EQueueFunction::Present>(device)->getQueueOfPriority<EQueuePriority::SuperHigh>(), &presentInfo
        );
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        LOG_ERROR("VulkanPresenting", "Failed to present images");
    }
    else
    {
        for (int32 i = 0; i < results.size(); ++i)
        {
            if (results[i] != VK_SUCCESS && results[i] != VK_SUBOPTIMAL_KHR)
            {
                LOG_ERROR("VulkanPresenting", "Failed presenting for window %s", canvases[i]->getResourceName().getChar());
            }
        }
    }
}

SemaphoreRef VulkanGraphicsHelper::createSemaphore(IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    auto *semaphore = new VulkanSemaphore(device);
    semaphore->setResourceName(semaphoreName);
    return SemaphoreRef(semaphore);
}

TimelineSemaphoreRef VulkanGraphicsHelper::createTimelineSemaphore(IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    auto *tSemaphore = new VulkanTimelineSemaphore(device);
    tSemaphore->setResourceName(semaphoreName);
    return TimelineSemaphoreRef(tSemaphore);
}

void VulkanGraphicsHelper::waitTimelineSemaphores(
    IGraphicsInstance *graphicsInstance, std::vector<TimelineSemaphoreRef> *semaphores, std::vector<uint64> *waitForValues
) const
{
    fatalAssertf(
        semaphores->size() <= waitForValues->size(), "Cannot wait on semaphores if the wait for values is less than waiting semaphors count"
    );

    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    std::vector<VkSemaphore> deviceSemaphores;
    deviceSemaphores.reserve(semaphores->size());
    for (const TimelineSemaphoreRef &semaphore : *semaphores)
    {
        deviceSemaphores.push_back(semaphore.reference<VulkanTimelineSemaphore>()->semaphore);
    }

    SEMAPHORE_WAIT_INFO(waitInfo);
    waitInfo.pSemaphores = deviceSemaphores.data();
    waitInfo.semaphoreCount = (uint32)deviceSemaphores.size();
    waitInfo.pValues = waitForValues->data();

    device->vkWaitSemaphores(device->logicalDevice, &waitInfo, 2000000000 /*2 Seconds*/);
}

FenceRef VulkanGraphicsHelper::createFence(IGraphicsInstance *graphicsInstance, const TChar *fenceName, bool bIsSignaled /*= false*/) const
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    VulkanFence *fence = new VulkanFence(device, bIsSignaled);
    fence->setResourceName(fenceName);
    return FenceRef(fence);
}

void VulkanGraphicsHelper::waitFences(IGraphicsInstance *graphicsInstance, std::vector<FenceRef> *fences, bool waitAll) const
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    std::vector<VkFence> deviceFences;
    deviceFences.reserve(fences->size());
    for (const FenceRef &fence : *fences)
    {
        deviceFences.push_back(fence.reference<VulkanFence>()->fence);
    }

    device->vkWaitForFences(
        device->logicalDevice, (uint32)deviceFences.size(), deviceFences.data(), waitAll ? VK_TRUE : VK_FALSE, 2000000000 /*2 Seconds*/
    );
}

VkBuffer VulkanGraphicsHelper::createBuffer(
    IGraphicsInstance *graphicsInstance, const VkBufferCreateInfo &bufferCreateInfo, EPixelDataFormat::Type bufferDataFormat
)
{
    VkBuffer buffer = nullptr;

    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    VkFormatFeatureFlags requiredFeatures
        = (bufferCreateInfo.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) > 0 ? VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT : 0;
    requiredFeatures
        |= (bufferCreateInfo.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) > 0 ? VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT : 0;
    if (requiredFeatures > 0)
    {
        const EPixelDataFormat::PixelFormatInfo *imageFormatInfo = EPixelDataFormat::getFormatInfo(bufferDataFormat);
        if (bufferDataFormat == EPixelDataFormat::Undefined || !imageFormatInfo)
        {
            LOG_ERROR("NewBufferCreation", "Invalid expected pixel format for buffer");
            return buffer;
        }

        VkFormatProperties formatProps;
        Vk::vkGetPhysicalDeviceFormatProperties(device->physicalDevice, EngineToVulkanAPI::vulkanDataFormat(bufferDataFormat), &formatProps);

        if ((formatProps.bufferFeatures & requiredFeatures) != requiredFeatures)
        {
            LOG_ERROR("NewBufferCreation", "Required format %s for buffer is not supported by device", imageFormatInfo->formatName);
            return buffer;
        }
    }

    if (device->vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        buffer = nullptr;
    }

    return buffer;
}

void VulkanGraphicsHelper::destroyBuffer(IGraphicsInstance *graphicsInstance, VkBuffer buffer)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyBuffer(device->logicalDevice, buffer, nullptr);
}

bool VulkanGraphicsHelper::allocateBufferResource(
    IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource, bool cpuAccessible
)
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    auto *resource = static_cast<VulkanBufferResource *>(memoryResource);
    VulkanMemoryAllocation allocation = gInstance->memoryAllocator->allocateBuffer(resource->buffer, cpuAccessible);
    if (allocation.memBlock)
    {
        memoryResource->setMemoryData(allocation);
        gInstance->selectedDevice.vkBindBufferMemory(
            gInstance->selectedDevice.logicalDevice, resource->buffer, memoryResource->getDeviceMemory(), memoryResource->allocationOffset()
        );
        return true;
    }
    return false;
}

void VulkanGraphicsHelper::deallocateBufferResource(IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource)
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    auto *resource = static_cast<VulkanBufferResource *>(memoryResource);
    if (memoryResource->getMemoryData().memBlock)
    {
        gInstance->memoryAllocator->deallocateBuffer(resource->buffer, memoryResource->getMemoryData());
    }
}

void VulkanGraphicsHelper::mapResource(IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const
{
    VulkanGraphicsInstance *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    IVulkanMemoryResources *memoryResource = static_cast<VulkanBufferResource *>(buffer.reference());

    if (memoryResource->getMappedMemory() == nullptr)
    {
        gInstance->memoryAllocator->mapBuffer(memoryResource->getMemoryData());
    }
}

void VulkanGraphicsHelper::unmapResource(IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    IVulkanMemoryResources *memoryResource = static_cast<VulkanBufferResource *>(buffer.reference());

    if (memoryResource->getMappedMemory() != nullptr)
    {
        gInstance->memoryAllocator->unmapBuffer(memoryResource->getMemoryData());
    }
}

void VulkanGraphicsHelper::mapResource(IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    IVulkanMemoryResources *memoryResource = static_cast<VulkanImageResource *>(image.reference());

    if (memoryResource->getMappedMemory() == nullptr && image->isStagingResource())
    {
        gInstance->memoryAllocator->mapImage(memoryResource->getMemoryData());
    }
}

void VulkanGraphicsHelper::unmapResource(IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    IVulkanMemoryResources *memoryResource = static_cast<VulkanImageResource *>(image.reference());

    if (memoryResource->getMappedMemory() != nullptr && image->isStagingResource())
    {
        gInstance->memoryAllocator->unmapImage(memoryResource->getMemoryData());
    }
}

VkBufferView VulkanGraphicsHelper::createBufferView(IGraphicsInstance *graphicsInstance, const VkBufferViewCreateInfo &viewCreateInfo)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    VkBufferView view;
    if (device->vkCreateBufferView(device->logicalDevice, &viewCreateInfo, nullptr, &view) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Buffer view creation failed");
        view = nullptr;
    }
    return view;
}

void VulkanGraphicsHelper::destroyBufferView(IGraphicsInstance *graphicsInstance, VkBufferView view)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyBufferView(device->logicalDevice, view, nullptr);
}

BufferResourceRef VulkanGraphicsHelper::createReadOnlyBuffer(IGraphicsInstance *, uint32 bufferStride, uint32 bufferCount /*= 1*/) const
{
    auto rBuffer = new VulkanRBuffer(bufferStride, bufferCount);
    return BufferResourceRef(rBuffer);
}

BufferResourceRef VulkanGraphicsHelper::createWriteOnlyBuffer(IGraphicsInstance *, uint32 bufferStride, uint32 bufferCount /*= 1*/) const
{
    auto wBuffer = new VulkanWBuffer(bufferStride, bufferCount);
    return BufferResourceRef(wBuffer);
}

BufferResourceRef VulkanGraphicsHelper::createReadWriteBuffer(IGraphicsInstance *, uint32 bufferStride, uint32 bufferCount /*= 1*/) const
{
    auto rwBuffer = new VulkanRWBuffer(bufferStride, bufferCount);
    return BufferResourceRef(rwBuffer);
}

BufferResourceRef
VulkanGraphicsHelper::createReadOnlyTexels(IGraphicsInstance *, EPixelDataFormat::Type texelFormat, uint32 bufferCount /*= 1*/) const
{
    auto rTexels = new VulkanRTexelBuffer(texelFormat, bufferCount);
    return BufferResourceRef(rTexels);
}

BufferResourceRef
VulkanGraphicsHelper::createWriteOnlyTexels(IGraphicsInstance *, EPixelDataFormat::Type texelFormat, uint32 bufferCount /*= 1*/) const
{
    auto wTexels = new VulkanWTexelBuffer(texelFormat, bufferCount);
    return BufferResourceRef(wTexels);
}

BufferResourceRef
VulkanGraphicsHelper::createReadWriteTexels(IGraphicsInstance *, EPixelDataFormat::Type texelFormat, uint32 bufferCount /*= 1*/) const
{
    auto rwTexels = new VulkanRWTexelBuffer(texelFormat, bufferCount);
    return BufferResourceRef(rwTexels);
}

BufferResourceRef VulkanGraphicsHelper::createReadOnlyIndexBuffer(IGraphicsInstance *, uint32 bufferStride, uint32 bufferCount /*= 1*/) const
{
    auto rIndexBuffer = new VulkanIndexBuffer(bufferStride, bufferCount);
    return BufferResourceRef(rIndexBuffer);
}

BufferResourceRef VulkanGraphicsHelper::createReadOnlyVertexBuffer(IGraphicsInstance *, uint32 bufferStride, uint32 bufferCount /*= 1*/) const
{
    auto rVertexBuffer = new VulkanVertexBuffer(bufferStride, bufferCount);
    return BufferResourceRef(rVertexBuffer);
}

BufferResourceRef VulkanGraphicsHelper::createReadOnlyIndirectBuffer(IGraphicsInstance *, uint32 bufferStride, uint32 bufferCount /*= 1*/) const
{
    auto rIndirectBuffer = new VulkanRIndirectBuffer(bufferStride, bufferCount);
    return BufferResourceRef(rIndirectBuffer);
}

BufferResourceRef
VulkanGraphicsHelper::createWriteOnlyIndirectBuffer(IGraphicsInstance *, uint32 bufferStride, uint32 bufferCount /*= 1*/) const
{
    auto wIndirectBuffer = new VulkanWIndirectBuffer(bufferStride, bufferCount);
    return BufferResourceRef(wIndirectBuffer);
}

VkImage
VulkanGraphicsHelper::createImage(IGraphicsInstance *graphicsInstance, VkImageCreateInfo &createInfo, VkFormatFeatureFlags &requiredFeatures)
{
    VkImage image = nullptr;

    const VulkanGraphicsInstance *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    if (requiredFeatures > 0)
    {
        VkFormatProperties pixelFormatProperties;
        Vk::vkGetPhysicalDeviceFormatProperties(device->physicalDevice, createInfo.format, &pixelFormatProperties);
        VkFormatFeatureFlags availableFeatures = createInfo.tiling == VK_IMAGE_TILING_LINEAR ? pixelFormatProperties.linearTilingFeatures
                                                                                             : pixelFormatProperties.optimalTilingFeatures;
        if ((availableFeatures & requiredFeatures) != requiredFeatures)
        {
            LOG_ERROR("NewImageCreation", "Required format for image is not supported by device");
            return image;
        }
    }

    VkImageFormatProperties imageFormatProperties;
    Vk::vkGetPhysicalDeviceImageFormatProperties(
        device->physicalDevice, createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags,
        &imageFormatProperties
    );
    if (imageFormatProperties.maxExtent.width < createInfo.extent.width || imageFormatProperties.maxExtent.height < createInfo.extent.height
        || imageFormatProperties.maxExtent.depth < createInfo.extent.depth)
    {
        LOG_ERROR(
            "NewImageCreation",
            "Image size (%d, %d, %d) is exceeding the maximum size (%d, %d, %d) supported by "
            "device",
            createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth, imageFormatProperties.maxExtent.width,
            imageFormatProperties.maxExtent.height, imageFormatProperties.maxExtent.depth
        );
        return image;
    }

    if (createInfo.arrayLayers > imageFormatProperties.maxArrayLayers)
    {
        LOG_WARN(
            "NewImageCreation",
            "Image layer count %d is exceeding the maximum layer count %d supported by "
            "device, using max limit",
            createInfo.arrayLayers, imageFormatProperties.maxArrayLayers
        );
        createInfo.arrayLayers = imageFormatProperties.maxArrayLayers;
    }

    if (createInfo.mipLevels > imageFormatProperties.maxMipLevels)
    {
        LOG_WARN(
            "NewImageCreation",
            "Image mip levels %d is exceeding the maximum mip levels %d supported by device, "
            "using max limit",
            createInfo.mipLevels, imageFormatProperties.maxMipLevels
        );
        createInfo.mipLevels = imageFormatProperties.maxMipLevels;
    }

    if (device->vkCreateImage(device->logicalDevice, &createInfo, nullptr, &image) != VK_SUCCESS)
    {
        image = nullptr;
    }
    return image;
}

void VulkanGraphicsHelper::destroyImage(IGraphicsInstance *graphicsInstance, VkImage image)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyImage(device->logicalDevice, image, nullptr);
}

bool VulkanGraphicsHelper::allocateImageResource(
    IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource, bool cpuAccessible
)
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    auto *resource = static_cast<VulkanImageResource *>(memoryResource);
    VulkanMemoryAllocation allocation = gInstance->memoryAllocator->allocateImage(
        resource->image, cpuAccessible,
        !resource->isStagingResource()
    ); // Every image apart from staging image are optimal

    if (allocation.memBlock)
    {
        memoryResource->setMemoryData(allocation);
        gInstance->selectedDevice.vkBindImageMemory(
            gInstance->selectedDevice.logicalDevice, resource->image, memoryResource->getDeviceMemory(), memoryResource->allocationOffset()
        );
        return true;
    }
    return false;
}

void VulkanGraphicsHelper::deallocateImageResource(IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource)
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    auto *resource = static_cast<VulkanImageResource *>(memoryResource);
    if (memoryResource->getMemoryData().memBlock)
    {
        gInstance->memoryAllocator->deallocateImage(
            resource->image, memoryResource->getMemoryData(),
            !resource->isStagingResource()
        ); // Every image apart from staging image are optimal
    }
}

VkImageView VulkanGraphicsHelper::createImageView(IGraphicsInstance *graphicsInstance, const VkImageViewCreateInfo &viewCreateInfo)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    VkImageView view;
    if (device->vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Image view creation failed");
        view = nullptr;
    }
    return view;
}

void VulkanGraphicsHelper::destroyImageView(IGraphicsInstance *graphicsInstance, VkImageView view)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyImageView(device->logicalDevice, view, nullptr);
}

ImageResourceRef VulkanGraphicsHelper::createImage(IGraphicsInstance *, ImageResourceCreateInfo createInfo, bool bIsStaging /*= false*/) const
{
    return ImageResourceRef(new VulkanImageResource(createInfo, bIsStaging));
}

ImageResourceRef
VulkanGraphicsHelper::createCubeImage(IGraphicsInstance *, ImageResourceCreateInfo createInfo, bool bIsStaging /*= false*/) const
{
    return ImageResourceRef(new VulkanCubeImageResource(createInfo, bIsStaging));
}

ImageResourceRef VulkanGraphicsHelper::createRTImage(
    IGraphicsInstance *, ImageResourceCreateInfo createInfo, EPixelSampleCount::Type sampleCount /*= EPixelSampleCount::SampleCount1*/
) const
{
    auto rtImage = new VulkanRenderTargetResource(createInfo);
    rtImage->setSampleCounts(sampleCount);
    return ImageResourceRef(rtImage);
}

ImageResourceRef VulkanGraphicsHelper::createCubeRTImage(
    IGraphicsInstance *, ImageResourceCreateInfo createInfo, EPixelSampleCount::Type sampleCount /*= EPixelSampleCount::SampleCount1*/
) const
{
    auto rtImage = new VulkanCubeRTImageResource(createInfo);
    rtImage->setSampleCounts(sampleCount);
    return ImageResourceRef(rtImage);
}

SamplerRef VulkanGraphicsHelper::createSampler(IGraphicsInstance *graphicsInstance, SamplerCreateInfo createInfo) const
{
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    VulkanSampler *sampler = new VulkanSampler(&gInstance->selectedDevice, createInfo);
    return SamplerRef(sampler);
}

ESamplerFiltering::Type VulkanGraphicsHelper::clampFiltering(
    IGraphicsInstance *graphicsInstance, ESamplerFiltering::Type sampleFiltering, EPixelDataFormat::Type imageFormat
) const
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    VkFormatProperties formatProps;
    Vk::vkGetPhysicalDeviceFormatProperties(device->physicalDevice, EngineToVulkanAPI::vulkanDataFormat(imageFormat), &formatProps);
    ESamplerFiltering::Type choosenFiltering = sampleFiltering;

    // Since filtering is done currently only on optimal tiled data
    while (choosenFiltering != ESamplerFiltering::Nearest)
    {
        VkFormatFeatureFlags requiredFeature = 0;
        switch (choosenFiltering)
        {
        case ESamplerFiltering::Nearest:
            requiredFeature |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            break;
        case ESamplerFiltering::Linear:
            requiredFeature |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
            break;
        case ESamplerFiltering::Cubic:
            requiredFeature |= VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG;
            break;
        default:
            LOG_ERROR("VulkanGraphicsHelper", "not supported filtering mode %s", ESamplerFiltering::filterName(choosenFiltering).getChar());
            choosenFiltering = ESamplerFiltering::Type(choosenFiltering - 1);
            continue;
        }

        if ((formatProps.optimalTilingFeatures & requiredFeature) == requiredFeature)
        {
            return choosenFiltering;
        }
        else
        {
            choosenFiltering = ESamplerFiltering::Type(choosenFiltering - 1);
        }
    }
    return choosenFiltering;
}

WindowCanvasRef VulkanGraphicsHelper::createWindowCanvas(IGraphicsInstance *, GenericAppWindow *fromWindow) const
{
    return WindowCanvasRef(new VulkanWindowCanvas(fromWindow));
}

void VulkanGraphicsHelper::cacheSurfaceProperties(IGraphicsInstance *graphicsInstance, const WindowCanvasRef &windowCanvas) const
{
    VulkanGraphicsInstance *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);

    if (!gInstance->selectedDevice.isValidDevice())
    {
        gInstance->createVulkanDevice(windowCanvas);
        fatalAssertf(gInstance->selectedDevice.isValidDevice(), "Graphics device creation failed");
    }
    gInstance->selectedDevice.cacheGlobalSurfaceProperties(windowCanvas);
}

void *VulkanGraphicsHelper::borrowMappedPtr(IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const
{
    auto *imgRes = static_cast<VulkanImageResource *>(resource.reference());
    mapResource(graphicsInstance, resource);
    return imgRes->getMappedMemory();
}

void VulkanGraphicsHelper::returnMappedPtr(IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const
{
    unmapResource(graphicsInstance, resource);
}

void VulkanGraphicsHelper::flushMappedPtr(IGraphicsInstance *graphicsInstance, const std::vector<ImageResourceRef> &resources) const
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    std::vector<VkMappedMemoryRange> memRanges;
    memRanges.reserve(resources.size());
    for (auto &resource : resources)
    {
        IVulkanMemoryResources *memRes = static_cast<VulkanImageResource *>(resource.reference());

        MAPPED_MEMORY_RANGE(memRange);
        memRange.memory = memRes->getDeviceMemory();
        memRange.size = memRes->allocatedSize();
        memRange.offset = memRes->allocationOffset();
        memRanges.emplace_back(memRange);
    }

    if (!memRanges.empty())
    {
        VkResult result = device->vkFlushMappedMemoryRanges(device->logicalDevice, uint32(memRanges.size()), memRanges.data());
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("VulkanGraphicsHelper", "failure in flushing mapped memories");
        }
    }
}

void *VulkanGraphicsHelper::borrowMappedPtr(IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const
{
    auto *bufferRes = static_cast<VulkanBufferResource *>(resource.reference());
    mapResource(graphicsInstance, resource);
    return bufferRes->getMappedMemory();
}

void VulkanGraphicsHelper::returnMappedPtr(IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const
{
    unmapResource(graphicsInstance, resource);
}

void VulkanGraphicsHelper::flushMappedPtr(IGraphicsInstance *graphicsInstance, const std::vector<BufferResourceRef> &resources) const
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    std::vector<VkMappedMemoryRange> memRanges;
    memRanges.reserve(resources.size());
    for (auto &resource : resources)
    {
        IVulkanMemoryResources *memRes = static_cast<VulkanBufferResource *>(resource.reference());

        MAPPED_MEMORY_RANGE(memRange);
        memRange.memory = memRes->getDeviceMemory();
        memRange.size = memRes->allocatedSize();
        memRange.offset = memRes->allocationOffset();
        memRanges.emplace_back(memRange);
    }

    if (!memRanges.empty())
    {
        VkResult result = device->vkFlushMappedMemoryRanges(device->logicalDevice, uint32(memRanges.size()), memRanges.data());
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("VulkanGraphicsHelper", "failure in flushing mapped memories");
        }
    }
}

void VulkanGraphicsHelper::markForDeletion(
    IGraphicsInstance *graphicsInstance, GraphicsResource *resource, EDeferredDelStrategy deleteStrategy, TickRep duration /*= 1*/
) const
{
    if (resource == nullptr)
    {
        return;
    }
#if DEFER_DELETION
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    VulkanDevice *device = &gInstance->selectedDevice;

    DeferredDeleter::DeferringData deferInfo{ .resource = resource, .elapsedDuration = 0, .strategy = deleteStrategy };
    switch (deleteStrategy)
    {
    case EDeferredDelStrategy::FrameCount:
        deferInfo.deferDuration = duration;
        break;
    case EDeferredDelStrategy::SwapchainCount:
        deferInfo.deferDuration = device->choosenImageCount;
        break;
    case EDeferredDelStrategy::TimePeriod:
        deferInfo.deferDuration = duration;
        deferInfo.elapsedDuration = Time::timeNow();
        break;
    case EDeferredDelStrategy::Immediate:
    default:
        break;
    }
    getDeferredDeleter(graphicsInstance)->deferDelete(std::move(deferInfo));
#else  // DEFER_DELETION
    resource->release();
    delete resource;
#endif // DEFER_DELETION
}

void VulkanGraphicsHelper::markForDeletion(
    IGraphicsInstance *graphicsInstance, SimpleSingleCastDelegate deleter, EDeferredDelStrategy deleteStrategy, TickRep duration /*= 1 */
) const
{
    if (!deleter.isBound())
    {
        return;
    }
#if DEFER_DELETION
    auto *gInstance = static_cast<VulkanGraphicsInstance *>(graphicsInstance);
    VulkanDevice *device = &gInstance->selectedDevice;

    DeferredDeleter::DeferringData deferInfo{ .deleter = deleter, .elapsedDuration = 0, .strategy = deleteStrategy };
    switch (deleteStrategy)
    {
    case EDeferredDelStrategy::FrameCount:
        deferInfo.deferDuration = duration;
        break;
    case EDeferredDelStrategy::SwapchainCount:
        deferInfo.deferDuration = device->choosenImageCount;
        break;
    case EDeferredDelStrategy::TimePeriod:
        deferInfo.deferDuration = duration;
        deferInfo.elapsedDuration = Time::timeNow();
        break;
    case EDeferredDelStrategy::Immediate:
    default:
        break;
    }
    getDeferredDeleter(graphicsInstance)->deferDelete(std::move(deferInfo));
#else  // DEFER_DELETION
    deleter.invoke();
#endif // DEFER_DELETION
}

VkShaderModule VulkanGraphicsHelper::createShaderModule(IGraphicsInstance *graphicsInstance, const uint8 *code, uint32 size)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    SHADER_MODULE_CREATE_INFO(createInfo);
    createInfo.codeSize = size;
    createInfo.pCode = reinterpret_cast<const uint32 *>(code);

    VkShaderModule shaderModule;
    if (device->vkCreateShaderModule(device->logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "failure in creating shader module[Shader size : %d]", size);
        shaderModule = nullptr;
    }
    return shaderModule;
}

void VulkanGraphicsHelper::destroyShaderModule(IGraphicsInstance *graphicsInstance, VkShaderModule shaderModule)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyShaderModule(device->logicalDevice, shaderModule, nullptr);
}

void VulkanGraphicsHelper::destroyRenderPass(IGraphicsInstance *graphicsInstance, VkRenderPass renderPass)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);
}

void VulkanGraphicsHelper::createFramebuffer(
    IGraphicsInstance *graphicsInstance, VkFramebufferCreateInfo &fbCreateInfo, VkFramebuffer *framebuffer
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    if (device->vkCreateFramebuffer(device->logicalDevice, &fbCreateInfo, nullptr, framebuffer) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Failed creating framebuffer");
        (*framebuffer) = nullptr;
    }
}

void VulkanGraphicsHelper::destroyFramebuffer(IGraphicsInstance *graphicsInstance, VkFramebuffer framebuffer)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyFramebuffer(device->logicalDevice, framebuffer, nullptr);
}

VkDescriptorSetLayout
VulkanGraphicsHelper::createDescriptorsSetLayout(IGraphicsInstance *graphicsInstance, const VkDescriptorSetLayoutCreateInfo &layoutCreateInfo)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    VkDescriptorSetLayout layout;
    if (device->vkCreateDescriptorSetLayout(device->logicalDevice, &layoutCreateInfo, nullptr, &layout) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Failed creating descriptor set layout");
        layout = nullptr;
    }
    return layout;
}

VkDescriptorSetLayout VulkanGraphicsHelper::getEmptyDescriptorsSetLayout(IGraphicsInstance *graphicsInstance)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);

    static VkDescriptorSetLayout layout = nullptr;
    if (!layout)
    {
        layout = gInstance->descriptorsSetAllocator->getEmptyLayout();
    }
    return layout;
}

void VulkanGraphicsHelper::destroyDescriptorsSetLayout(IGraphicsInstance *graphicsInstance, VkDescriptorSetLayout descriptorsSetLayout)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorsSetLayout, nullptr);
}

void VulkanGraphicsHelper::updateDescriptorsSet(
    IGraphicsInstance *graphicsInstance, const std::vector<VkWriteDescriptorSet> &writingDescriptors,
    const std::vector<VkCopyDescriptorSet> &copyingDescsSets
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    device->vkUpdateDescriptorSets(
        device->logicalDevice, uint32(writingDescriptors.size()), writingDescriptors.data(), uint32(copyingDescsSets.size()),
        copyingDescsSets.data()
    );
}

void VulkanGraphicsHelper::destroyPipelineLayout(IGraphicsInstance *graphicsInstance, const VkPipelineLayout pipelineLayout)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    device->vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
}

VkPipelineCache VulkanGraphicsHelper::createPipelineCache(IGraphicsInstance *graphicsInstance, ArrayView<const uint8> cacheData)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    // Validate cacheData
    static_assert(sizeof(VkPipelineCacheHeaderVersionOne) == 32, "Mismatching size of Cache header size");
    const VkPipelineCacheHeaderVersionOne *cacheHeader = reinterpret_cast<const VkPipelineCacheHeaderVersionOne *>(cacheData.data());
    if (cacheData.empty() || cacheData.size() < sizeof(VkPipelineCacheHeaderVersionOne) || cacheHeader->headerSize != 32)
    {
        LOG_ERROR(
            "VulkanGraphicsHelper", "Invalid pipeline cache data, Pipeline cache cannot be created! Use "
                                    "\"createPipelineCache(IGraphicsInstance *)\" to create an empty cache"
        );
        return nullptr;
    }
    else if (cacheHeader->deviceID != device->properties.deviceID 
        || cacheHeader->vendorID != device->properties.vendorID 
        || memcmp(cacheHeader->pipelineCacheUUID, device->properties.pipelineCacheUUID, VK_UUID_SIZE) != 0)
    {
        LOG("VulkanGraphicsHelper", "Pipelines cache mismatch, Cache is not created!");
        return nullptr;
    }

    // Create cache from cacheData
    PIPELINE_CACHE_CREATE_INFO(cacheCreateInfo);
    cacheCreateInfo.initialDataSize = cacheData.size();
    cacheCreateInfo.pInitialData = cacheData.data();
    VkPipelineCache pipelineCache;
    if (device->vkCreatePipelineCache(device->logicalDevice, &cacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Pipeline cache creation failed");
        pipelineCache = nullptr;
    }
    return pipelineCache;
}

VkPipelineCache VulkanGraphicsHelper::createPipelineCache(IGraphicsInstance *graphicsInstance)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    PIPELINE_CACHE_CREATE_INFO(cacheCreateInfo);

    VkPipelineCache pipelineCache;
    if (device->vkCreatePipelineCache(device->logicalDevice, &cacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Pipeline cache creation failed");
        pipelineCache = nullptr;
    }
    return pipelineCache;
}

void VulkanGraphicsHelper::destroyPipelineCache(IGraphicsInstance *graphicsInstance, VkPipelineCache pipelineCache)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyPipelineCache(device->logicalDevice, pipelineCache, nullptr);
}

void VulkanGraphicsHelper::mergePipelineCaches(
    IGraphicsInstance *graphicsInstance, VkPipelineCache dstCache, ArrayView<const VkPipelineCache> srcCaches
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    if (device->vkMergePipelineCaches(device->logicalDevice, dstCache, uint32(srcCaches.size()), srcCaches.data()) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanGraphicsHelper", "Merging pipeline caches failed");
    }
}

void VulkanGraphicsHelper::getPipelineCacheData(
    IGraphicsInstance *graphicsInstance, VkPipelineCache pipelineCache, std::vector<uint8> &cacheData
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#pipelines-cache-header
    static_assert(sizeof(VkPipelineCacheHeaderVersionOne) == 32, "Mismatching size of Cache header size");

    uint64 cacheDataSize;
    device->vkGetPipelineCacheData(device->logicalDevice, pipelineCache, &cacheDataSize, nullptr);
    if (cacheDataSize >= sizeof(VkPipelineCacheHeaderVersionOne))
    {
        cacheData.resize(cacheDataSize);
        device->vkGetPipelineCacheData(device->logicalDevice, pipelineCache, &cacheDataSize, cacheData.data());
    }
}

std::vector<VkPipeline> VulkanGraphicsHelper::createGraphicsPipeline(
    IGraphicsInstance *graphicsInstance, const std::vector<VkGraphicsPipelineCreateInfo> &graphicsPipelineCI, VkPipelineCache pipelineCache
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    std::vector<VkPipeline> pipelines(graphicsPipelineCI.size());
    fatalAssertf(
        device->vkCreateGraphicsPipelines(
            device->logicalDevice, pipelineCache, uint32(graphicsPipelineCI.size()), graphicsPipelineCI.data(), nullptr, pipelines.data()
        ) == VK_SUCCESS,
        "Graphics pipeline creation failed"
    );

    return pipelines;
}

std::vector<VkPipeline> VulkanGraphicsHelper::createComputePipeline(
    IGraphicsInstance *graphicsInstance, const std::vector<VkComputePipelineCreateInfo> &computePipelineCI, VkPipelineCache pipelineCache
)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;

    std::vector<VkPipeline> pipelines(computePipelineCI.size());
    fatalAssertf(
        device->vkCreateComputePipelines(
            device->logicalDevice, pipelineCache, uint32(computePipelineCI.size()), computePipelineCI.data(), nullptr, pipelines.data()
        ) == VK_SUCCESS,
        "Compute pipeline creation failed"
    );

    return pipelines;
}

void VulkanGraphicsHelper::destroyPipeline(IGraphicsInstance *graphicsInstance, VkPipeline pipeline)
{
    const auto *gInstance = static_cast<const VulkanGraphicsInstance *>(graphicsInstance);
    const VulkanDevice *device = &gInstance->selectedDevice;
    device->vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
}

PipelineBase *VulkanGraphicsHelper::createGraphicsPipeline(IGraphicsInstance *, const PipelineBase *parent) const
{
    return new VulkanGraphicsPipeline(static_cast<const GraphicsPipelineBase *>(parent));
}

PipelineBase *VulkanGraphicsHelper::createGraphicsPipeline(IGraphicsInstance *, const GraphicsPipelineConfig &config) const
{
    auto *graphicsPipeline = new VulkanGraphicsPipeline();
    graphicsPipeline->setPipelineConfig(config);
    return graphicsPipeline;
}

PipelineBase *VulkanGraphicsHelper::createComputePipeline(IGraphicsInstance *, const PipelineBase *parent) const
{
    return new VulkanComputePipeline(static_cast<const ComputePipelineBase *>(parent));
}

PipelineBase *VulkanGraphicsHelper::createComputePipeline(IGraphicsInstance *) const { return new VulkanComputePipeline(); }

GlobalRenderingContextBase *VulkanGraphicsHelper::createGlobalRenderingContext() const { return new VulkanGlobalRenderingContext(); }

ShaderResource *VulkanGraphicsHelper::createShaderResource(const ShaderConfigCollector *inConfig) const
{
    return new VulkanShaderResource(inConfig);
}

ShaderParametersRef VulkanGraphicsHelper::createShaderParameters(
    IGraphicsInstance *, const GraphicsResource *paramLayout, const std::set<uint32> &ignoredSetIds /*= {}*/
) const
{
    ShaderParametersRef shaderParameter;
    if (paramLayout->getType()->isChildOf<ShaderSetParametersLayout>())
    {
        shaderParameter = new VulkanShaderSetParameters(paramLayout);
    }
    else if (paramLayout->getType()->isChildOf<ShaderParametersLayout>())
    {
        shaderParameter = new VulkanShaderParameters(paramLayout, ignoredSetIds);
    }
    return shaderParameter;
}

const GraphicsResourceType *VulkanGraphicsHelper::readOnlyBufferType() const { return VulkanRBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::writeOnlyBufferType() const { return VulkanWBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::readWriteBufferType() const { return VulkanRWBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::readOnlyTexelsType() const { return VulkanRTexelBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::writeOnlyTexelsType() const { return VulkanWTexelBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::readWriteTexelsType() const { return VulkanRWTexelBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::readOnlyIndexBufferType() const { return VulkanIndexBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::readOnlyVertexBufferType() const { return VulkanVertexBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::readOnlyIndirectBufferType() const { return VulkanRIndirectBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::writeOnlyIndirectBufferType() const { return VulkanWIndirectBuffer::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::imageType() const { return VulkanImageResource::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::cubeImageType() const { return VulkanCubeImageResource::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::rtImageType() const { return VulkanRenderTargetResource::staticType(); }

const GraphicsResourceType *VulkanGraphicsHelper::cubeRTImageType() const { return VulkanCubeRTImageResource::staticType(); }
