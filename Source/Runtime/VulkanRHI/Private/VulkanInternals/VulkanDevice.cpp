/*!
 * \file VulkanDevice.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/VulkanDevice.h"
#include "ApplicationSettings.h"
#include "Logger/Logger.h"
#include "Math/Math.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "Resources/VulkanWindowCanvas.h"
#include "String/String.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanInternals/Resources/VulkanQueueResource.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "VulkanInternals/VulkanMacros.h"

#include <set>
#include <sstream>

void VulkanDevice::markEnabledFeatures()
{
    // TODO(Jeslas) : Check and enable necessary features on enabledFeatures
    enabledFeatures.samplerAnisotropy = features.samplerAnisotropy;
    enabledFeatures.fillModeNonSolid = features.fillModeNonSolid;
    enabledFeatures.wideLines = features.wideLines;
    enabledFeatures.shaderStorageImageExtendedFormats = features.shaderStorageImageExtendedFormats;
    enabledFeatures.geometryShader = features.geometryShader;

    // Dynamic resource array
    enabledFeatures.shaderSampledImageArrayDynamicIndexing = features.shaderSampledImageArrayDynamicIndexing;
    enabledFeatures.shaderStorageImageArrayDynamicIndexing = features.shaderStorageImageArrayDynamicIndexing;
    PHYSICAL_DEVICE_DESC_INDEXING_FEATURES(descIdxFeatures);
    descIdxFeatures.shaderStorageTexelBufferArrayDynamicIndexing = descIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing;
    descIdxFeatures.shaderUniformTexelBufferArrayDynamicIndexing = descIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing;
    // Partial bindings
    descIdxFeatures.descriptorBindingPartiallyBound = descIndexingFeatures.descriptorBindingPartiallyBound;
    // Update after binding
    descIdxFeatures.descriptorBindingSampledImageUpdateAfterBind = descIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind;
    descIdxFeatures.descriptorBindingStorageImageUpdateAfterBind = descIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind;
    descIdxFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind
        = descIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind;
    descIdxFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind
        = descIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind;
    // Update unused
    descIdxFeatures.descriptorBindingUpdateUnusedWhilePending = descIndexingFeatures.descriptorBindingUpdateUnusedWhilePending;
    // Non uniform access to resource array
    descIdxFeatures.shaderSampledImageArrayNonUniformIndexing = descIndexingFeatures.shaderSampledImageArrayNonUniformIndexing;
    descIdxFeatures.shaderStorageImageArrayNonUniformIndexing = descIndexingFeatures.shaderStorageImageArrayNonUniformIndexing;
    descIdxFeatures.shaderUniformTexelBufferArrayNonUniformIndexing = descIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing;
    descIdxFeatures.shaderStorageTexelBufferArrayNonUniformIndexing = descIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing;
    // Runtime arrays
    descIdxFeatures.runtimeDescriptorArray = descIndexingFeatures.runtimeDescriptorArray;
    enabledDescIndexingFeatures = std::move(descIdxFeatures);

    // Multi draw
    enabledFeatures.multiDrawIndirect = features.multiDrawIndirect;
}

void VulkanDevice::markGlobalConstants()
{
    // Anisotropy
    if (enabledFeatures.samplerAnisotropy)
    {
        GlobalRenderVariables::ENABLE_ANISOTROPY.set(true);
        GlobalRenderVariables::MAX_ANISOTROPY.set(properties.limits.maxSamplerAnisotropy);
    }
    else
    {
        GlobalRenderVariables::ENABLE_ANISOTROPY.set(false);
        GlobalRenderVariables::MAX_ANISOTROPY.set(1);
    }

    GlobalRenderVariables::ENABLE_NON_FILL_DRAWS.set(enabledFeatures.fillModeNonSolid);
    GlobalRenderVariables::ENABLE_WIDE_LINES.set(enabledFeatures.wideLines);

    GlobalRenderVariables::ENABLED_RESOURCE_RUNTIME_ARRAY.set(enabledDescIndexingFeatures.runtimeDescriptorArray);
    GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_AFTER_BIND.set(
        enabledDescIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind
        && enabledDescIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind
        && enabledDescIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind
        && enabledDescIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind
    );
    GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_UNUSED.set(enabledDescIndexingFeatures.descriptorBindingUpdateUnusedWhilePending);
    GlobalRenderVariables::MAX_UPDATE_AFTER_BIND_DESCRIPTORS.set(descIndexingProps.maxUpdateAfterBindDescriptorsInAllPools);

    GlobalRenderVariables::MAX_INDIRECT_DRAW_COUNT.set(properties.limits.maxDrawIndirectCount);

    // Sync resources
    GlobalRenderVariables::MAX_TIMELINE_OFFSET.set(timelineSemaphoreProps.maxTimelineSemaphoreValueDifference);
    GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.set(timelineSemaphoreFeatures.timelineSemaphore);

    // Sampling texture

    // Storing resources
    GlobalRenderVariables::ENABLE_EXTENDED_STORAGES.set(enabledFeatures.shaderStorageImageExtendedFormats);

    GlobalRenderVariables::ENABLE_GEOMETRY_SHADERS.set(enabledFeatures.geometryShader);
}

bool VulkanDevice::createQueueResources()
{
    if (!GlobalRenderVariables::GPU_IS_COMPUTE_ONLY.get())
    {
        graphicsQueue = new VulkanQueueResource<EQueueFunction::Graphics>(queueFamiliesSupported);
        if (graphicsQueue->isValidQueue())
        {
            allQueues.push_back(graphicsQueue);
        }
        else
        {
            delete graphicsQueue;
            graphicsQueue = nullptr;
        }
    }

    computeQueue = new VulkanQueueResource<EQueueFunction::Compute>(queueFamiliesSupported);
    if (computeQueue->isValidQueue())
    {
        allQueues.push_back(computeQueue);
    }
    else
    {
        delete computeQueue;
        computeQueue = nullptr;
    }

    transferQueue = new VulkanQueueResource<EQueueFunction::Transfer>(queueFamiliesSupported);
    if (transferQueue->isValidQueue())
    {
        allQueues.push_back(transferQueue);
    }
    else
    {
        delete transferQueue;
        transferQueue = nullptr;
    }

    if (allQueues.size() != 3)
    {
        genericQueue = new VulkanQueueResource<EQueueFunction::Generic>(queueFamiliesSupported);
        if (genericQueue->isValidQueue())
        {
            allQueues.push_back(genericQueue);
        }
        else
        {
            delete genericQueue;
            genericQueue = nullptr;
            LOG_ERROR("VulkanDevice", "Failed creating necessary queue resources");
            return false;
        }
    }

    if (GlobalRenderVariables::PRESENTING_ENABLED.get())
    {
        alertAlwaysf(!GlobalRenderVariables::GPU_IS_COMPUTE_ONLY.get(), "Presenting enabled while GPU is used for compute only");

        if (presentQueues.empty())
        {
            LOG_ERROR("VulkanDevice", "No valid surface found, Skipping creating presentation queue");
        }
        else
        {
            std::map<uint32, VkQueueFamilyProperties *> supportedQueues;
            for (const uint32 &qIdx : presentQueues)
            {
                supportedQueues.insert({ qIdx, &queueFamiliesSupported[qIdx] });
            }

            QueueResourceBase *q = new VulkanQueueResource<EQueueFunction::Present>(supportedQueues);
            if (q->isValidQueue())
            {
                allQueues.push_back(q);
            }
            else
            {
                delete q;
            }
        }
    }

    return true;
}

bool VulkanDevice::collectDeviceExtensions(std::vector<const char *> &extensions) const
{
    extensions.clear();

    std::set<std::string::const_pointer> mandatoryExtensions;

#define DEVICE_VK_EXT_FUNCTIONS(function, extension) mandatoryExtensions.insert(extension);

#include "VulkanFunctionLists.inl"

    std::stringstream extensionsStream;
    for (const VkExtensionProperties &extProperty : availableExtensions)
    {
        extensionsStream << extProperty.extensionName;
    }
    std::string extensionsString = extensionsStream.str();

    for (const auto *mandatoryExt : mandatoryExtensions)
    {
        if (extensionsString.find(mandatoryExt, 0) != std::string::npos)
        {
            extensions.push_back(mandatoryExt);
            LOG_DEBUG("VulkanDevice", "Loading device extension %s", mandatoryExt);
        }
    }

    if (mandatoryExtensions.size() != extensions.size())
    {
        LOG_ERROR("VulkanDevice", "Missing mandatory extensions");
        return false;
    }

    return true;
}

#if DEV_BUILD
void VulkanDevice::collectDeviceLayers(std::vector<const char *> &layers) const {}
#endif

void VulkanDevice::loadDeviceFunctions()
{
#define DEVICE_VK_FUNCTIONS(function)                                                                                                          \
    function = (PFN_##function)Vk::vkGetDeviceProcAddr(logicalDevice, #function);                                                              \
    if (function == nullptr)                                                                                                                   \
        LOG_ERROR("VulkanDevice", "Failed loading function : " #function);

#define DEVICE_VK_EXT_FUNCTIONS(function, extension)                                                                                           \
    for (const char *ext : registeredExtensions)                                                                                               \
    {                                                                                                                                          \
        if (std::strcmp(ext, extension) == 0)                                                                                                  \
        {                                                                                                                                      \
            function = (PFN_##function)Vk::vkGetDeviceProcAddr(logicalDevice, #function);                                                      \
            break;                                                                                                                             \
        }                                                                                                                                      \
    }                                                                                                                                          \
    if (function == nullptr)                                                                                                                   \
        LOG_ERROR("VulkanDevice", "Failed loading function : " #function);

#include "VulkanFunctionLists.inl"
}

const std::vector<VulkanDevice::QueueResourceBasePtr> &getAllQueues(const VulkanDevice *device) { return device->allQueues; }

template <EQueueFunction QueueFunction>
VulkanQueueResource<QueueFunction> *getQueue(const VulkanDevice *device)
{
    for (QueueResourceBase *queue : getAllQueues(device))
    {
        if (queue->getType()->isChildOf<VulkanQueueResource<QueueFunction>>())
        {
            return static_cast<VulkanQueueResource<QueueFunction> *>(queue);
        }
    }
    // User needs to handle this case, We do not have to return generic queue
    return nullptr;
}

template <>
VulkanQueueResource<EQueueFunction::Compute> *getQueue<EQueueFunction::Compute>(const VulkanDevice *device)
{
    return static_cast<VulkanQueueResource<EQueueFunction::Compute> *>(device->getComputeQueue());
}
template <>
VulkanQueueResource<EQueueFunction::Generic> *getQueue<EQueueFunction::Generic>(const VulkanDevice *device)
{
    return static_cast<VulkanQueueResource<EQueueFunction::Generic> *>(device->getGenericQueue());
}
template <>
VulkanQueueResource<EQueueFunction::Graphics> *getQueue<EQueueFunction::Graphics>(const VulkanDevice *device)
{
    return static_cast<VulkanQueueResource<EQueueFunction::Graphics> *>(device->getGraphicsQueue());
}
template <>
VulkanQueueResource<EQueueFunction::Transfer> *getQueue<EQueueFunction::Transfer>(const VulkanDevice *device)
{
    return static_cast<VulkanQueueResource<EQueueFunction::Transfer> *>(device->getTransferQueue());
}

void VulkanDevice::cacheGlobalSurfaceProperties(const WindowCanvasRef &windowCanvas)
{
    // Window surface is invalid, Then we don't present
    if (!windowCanvas.isValid())
    {
        return;
    }
    const VulkanWindowCanvas *canvas = windowCanvas.reference<VulkanWindowCanvas>();

    // If queues are not yet created cache present queues for this surface
    if (!getQueue<EQueueFunction::Present>(this))
    {
        // Cache queues
        presentQueues.clear();
        for (int32 index = 0; index < queueFamiliesSupported.size(); ++index)
        {
            VkBool32 queueSupported;
            Vk::vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, canvas->surface(), &queueSupported);
            if (queueSupported)
            {
                presentQueues.emplace_back(index);
            }
        }
        fatalAssertf(!presentQueues.empty(), "Window is available but no queues support presenting to the window surface");
    }
    GlobalRenderVariables::PRESENTING_ENABLED.set(true);
    alertAlwaysf(!GlobalRenderVariables::GPU_IS_COMPUTE_ONLY.get(), "Presenting must not be enabled in compute only device!");

    VkSurfaceCapabilitiesKHR swapchainCapabilities;
    Vk::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, canvas->surface(), &swapchainCapabilities);

    choosenImageCount = swapchainCapabilities.minImageCount + 1;

    // Presentation mode
    {
        uint32 presentModesCount;
        Vk::vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, canvas->surface(), (uint32_t *)&presentModesCount, nullptr);

        std::vector<VkPresentModeKHR> presentModes(presentModesCount);
        Vk::vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, canvas->surface(), (uint32_t *)&presentModesCount, presentModes.data());
        if (ApplicationSettings::enableVsync.get())
        {
            fatalAssertf(
                std::find(presentModes.cbegin(), presentModes.cend(), VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR) != presentModes.cend(),
                "V-Sync not supported"
            );
            globalPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
            LOG_DEBUG("VulkanDevice", "Choosen fifo present mode");
            choosenImageCount = swapchainCapabilities.minImageCount;
        }
        else if (std::find(presentModes.cbegin(), presentModes.cend(), VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.cend())
        {
            globalPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
            LOG_DEBUG("VulkanDevice", "Choosen mailbox present mode");
            choosenImageCount = Math::max<uint32>(choosenImageCount, 3);
        }
        else if (std::find(presentModes.cbegin(), presentModes.cend(), VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR) != presentModes.cend())
        {
            globalPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            LOG_DEBUG("VulkanDevice", "Choosen fifo relaxed present mode");
            choosenImageCount = Math::max<uint32>(choosenImageCount, 3);
        }
        else
        {
            fatalAssertf(
                std::find(presentModes.cbegin(), presentModes.cend(), VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR) != presentModes.cend(),
                "No accepted present mode is found, not even default case"
            );
            globalPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
            LOG_DEBUG("VulkanDevice", "Choosen fifo present mode");
            choosenImageCount = Math::max<uint32>(choosenImageCount, 2);
        }
    }

    if (swapchainCapabilities.maxImageCount > 0)
    {
        choosenImageCount = Math::min<uint32>(choosenImageCount, swapchainCapabilities.maxImageCount);
    }
    swapchainImgUsage = swapchainCapabilities.supportedUsageFlags
                        & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    // Surface formats
    {
        uint32 formatCount;
        Vk::vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, canvas->surface(), (uint32_t *)&formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formatsSupported(formatCount);
        Vk::vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, canvas->surface(), (uint32_t *)&formatCount, formatsSupported.data());

        debugAssert(formatCount > 0);
        swapchainFormat = formatsSupported[0];
    }
}

int32 VulkanDevice::compareSurfaceCompatibility(const WindowCanvasRef &windowCanvas, const VulkanDevice &otherDevice) const
{
    const VulkanWindowCanvas *vkCanvas = windowCanvas.reference<VulkanWindowCanvas>();

    int32 presentationSupported;
    for (int32 index = 0; index < queueFamiliesSupported.size(); ++index)
    {
        VkBool32 queueSupported;
        Vk::vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, vkCanvas->surface(), &queueSupported);
        presentationSupported = queueSupported == 0 ? 0 : 1;
    }

    int32 otherPresentationSupported;
    for (int32 index = 0; index < otherDevice.queueFamiliesSupported.size(); ++index)
    {
        VkBool32 queueSupported;
        Vk::vkGetPhysicalDeviceSurfaceSupportKHR(otherDevice.physicalDevice, index, vkCanvas->surface(), &queueSupported);
        otherPresentationSupported = queueSupported == 0 ? 0 : 1;
    }
    return presentationSupported - otherPresentationSupported;
}

int32 VulkanDevice::compareMemoryCompatibility(const VulkanDevice &otherDevice) const
{
    int32 heapCountDiff = ((int32)memoryProperties.memoryHeapCount) - ((int32)otherDevice.memoryProperties.memoryHeapCount);
    if (heapCountDiff != 0)
    {
        return heapCountDiff > 0 ? 1 : -1;
    }

    int64 maxSupportedDeviceHeapSize = 0;
    int32 deviceHeapIndex = -1;
    int32 sharedHeapIndex = -1;
    for (uint32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if (memoryProperties.memoryTypes[i].propertyFlags != 0
            && (memoryProperties.memoryTypes[i].propertyFlags & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0)
        {
            if (maxSupportedDeviceHeapSize < (int64)memoryProperties.memoryHeaps[memoryProperties.memoryTypes[i].heapIndex].size)
            {
                deviceHeapIndex = memoryProperties.memoryTypes[i].heapIndex;
                maxSupportedDeviceHeapSize = memoryProperties.memoryHeaps[deviceHeapIndex].size;
            }
        }

        if ((memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        {
            sharedHeapIndex = memoryProperties.memoryTypes[i].heapIndex;
        }
    }

    int64 otherMaxSupportedDeviceHeapSize = 0;
    int32 otherDeviceHeapIndex = -1;
    int32 otherSharedHeapIndex = -1;
    for (uint32 i = 0; i < otherDevice.memoryProperties.memoryTypeCount; ++i)
    {
        if (otherDevice.memoryProperties.memoryTypes[i].propertyFlags != 0
            && (otherDevice.memoryProperties.memoryTypes[i].propertyFlags & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0)
        {
            if (otherMaxSupportedDeviceHeapSize
                < (int64)otherDevice.memoryProperties.memoryHeaps[otherDevice.memoryProperties.memoryTypes[i].heapIndex].size)
            {
                otherDeviceHeapIndex = otherDevice.memoryProperties.memoryTypes[i].heapIndex;
                otherMaxSupportedDeviceHeapSize = otherDevice.memoryProperties.memoryHeaps[otherDeviceHeapIndex].size;
            }
        }

        if ((otherDevice.memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        {
            otherSharedHeapIndex = otherDevice.memoryProperties.memoryTypes[i].heapIndex;
        }
    }

    // If both has shared memory only or both are dedicated
    if ((deviceHeapIndex == sharedHeapIndex && otherDeviceHeapIndex == otherSharedHeapIndex)
        || (deviceHeapIndex != sharedHeapIndex && otherDeviceHeapIndex != otherSharedHeapIndex))
    {
        int64 heapSizeDiff = maxSupportedDeviceHeapSize - otherMaxSupportedDeviceHeapSize;
        if (heapSizeDiff != 0)
        {
            return heapSizeDiff > 0 ? 1 : -1;
        }
    }

    if (deviceHeapIndex != sharedHeapIndex)
    {
        return 1;
    }
    if (otherDeviceHeapIndex != otherSharedHeapIndex)
    {
        return -1;
    }
    return 0;
}

VulkanDevice::VulkanDevice(VkPhysicalDevice &&device)
    : graphicsDebug(this)
{
    physicalDevice = device;
    uint32 extCount = 0;
    if (Vk::vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, (uint32_t *)&extCount, nullptr) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanDevice", "enumerating extensions for device failed");
        return;
    }
    availableExtensions.resize(extCount);
    Vk::vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, (uint32_t *)&extCount, availableExtensions.data());

    uint32 layerCount = 0;
    if (Vk::vkEnumerateDeviceLayerProperties(physicalDevice, (uint32_t *)&layerCount, nullptr) != VK_SUCCESS)
    {
        LOG_WARN("VulkanDevice", "enumerating layers for device failed");
    }
    else
    {
        availableLayers.resize(layerCount);
        Vk::vkEnumerateDeviceLayerProperties(physicalDevice, (uint32_t *)&layerCount, availableLayers.data());
    }

    { // Features
        PHYSICAL_DEVICE_FEATURES_2(advancedFeatures);
        PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES(tSemaphoreFeatures);
        advancedFeatures.pNext = &tSemaphoreFeatures;
        PHYSICAL_DEVICE_DESC_INDEXING_FEATURES(tDescIdxFeatures);
        tSemaphoreFeatures.pNext = &tDescIdxFeatures;
        PHYSICAL_DEVICE_SYNC_2_FEATURES_KHR(tSync2Features);
        tDescIdxFeatures.pNext = &tSync2Features;
        Vk::vkGetPhysicalDeviceFeatures2KHR(physicalDevice, &advancedFeatures);

        features = std::move(advancedFeatures.features);
        timelineSemaphoreFeatures = std::move(tSemaphoreFeatures);
        descIndexingFeatures = std::move(tDescIdxFeatures);
        sync2Features = std::move(tSync2Features);
        markEnabledFeatures();
    }

    { // Properties
        PHYSICAL_DEVICE_PROPERTIES_2(advancedProperties);
        PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES(tSemaphoreProperties);
        advancedProperties.pNext = &tSemaphoreProperties;
        PHYSICAL_DEVICE_DESC_INDEXING_PROPERTIES(descIdxProps);
        tSemaphoreProperties.pNext = &descIdxProps;
        Vk::vkGetPhysicalDeviceProperties2KHR(physicalDevice, &advancedProperties);

        properties = std::move(advancedProperties.properties);
        timelineSemaphoreProps = std::move(tSemaphoreProperties);
        descIndexingProps = std::move(descIdxProps);

        Vk::vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        LOG_DEBUG(
            "VulkanDevice", "Found %d memory types and %d heaps in device %s", memoryProperties.memoryTypeCount,
            memoryProperties.memoryHeapCount, properties.deviceName
        );
    }

    LOG_DEBUG("VulkanDevice", "Found %d extensions and %d layers in device %s", extCount, layerCount, properties.deviceName);
    LOG_DEBUG(
        "VulkanDevice", "Device API version %d.%d.%d Driver version %d.%d.%d", VK_VERSION_MAJOR(properties.apiVersion),
        VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion), VK_VERSION_MAJOR(properties.driverVersion),
        VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion)
    );

    uint32 queueCount;
    Vk::vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, (uint32_t *)&queueCount, nullptr);
    queueFamiliesSupported.resize(queueCount);
    Vk::vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, (uint32_t *)&queueCount, queueFamiliesSupported.data());
    LOG_DEBUG("VulkanDevice", "%s supports %d number of queue families", properties.deviceName, queueCount);
}

#define MOVE_IMPL()                                                                                                                            \
    availableExtensions = std::move(rVulkanDevice.availableExtensions);                                                                        \
    availableLayers = std::move(rVulkanDevice.availableLayers);                                                                                \
    features = std::move(rVulkanDevice.features);                                                                                              \
    properties = std::move(rVulkanDevice.properties);                                                                                          \
    queueFamiliesSupported = std::move(rVulkanDevice.queueFamiliesSupported);                                                                  \
    physicalDevice = std::move(rVulkanDevice.physicalDevice);                                                                                  \
    graphicsQueue = std::move(rVulkanDevice.graphicsQueue);                                                                                    \
    transferQueue = std::move(rVulkanDevice.transferQueue);                                                                                    \
    computeQueue = std::move(rVulkanDevice.computeQueue);                                                                                      \
    genericQueue = std::move(rVulkanDevice.genericQueue);                                                                                      \
    allQueues = std::move(rVulkanDevice.allQueues);                                                                                            \
    enabledFeatures = std::move(rVulkanDevice.enabledFeatures);                                                                                \
    logicalDevice = std::move(rVulkanDevice.logicalDevice);                                                                                    \
    timelineSemaphoreProps = std::move(rVulkanDevice.timelineSemaphoreProps);                                                                  \
    timelineSemaphoreFeatures = std::move(rVulkanDevice.timelineSemaphoreFeatures);                                                            \
    memoryProperties = std::move(rVulkanDevice.memoryProperties);                                                                              \
    enabledDescIndexingFeatures = std::move(rVulkanDevice.enabledDescIndexingFeatures);                                                        \
    descIndexingFeatures = std::move(rVulkanDevice.descIndexingFeatures);                                                                      \
    descIndexingProps = std::move(rVulkanDevice.descIndexingProps);                                                                            \
    sync2Features = std::move(rVulkanDevice.sync2Features);                                                                                    \
    graphicsDebug = { this };

VulkanDevice::VulkanDevice(VulkanDevice &&rVulkanDevice) { MOVE_IMPL(); }

void VulkanDevice::operator=(VulkanDevice &&rVulkanDevice) { MOVE_IMPL(); }
#undef MOVE_IMPL

#define COPY_IMPL()                                                                                                                            \
    availableExtensions = otherDevice.availableExtensions;                                                                                     \
    availableLayers = otherDevice.availableLayers;                                                                                             \
    features = otherDevice.features;                                                                                                           \
    properties = otherDevice.properties;                                                                                                       \
    queueFamiliesSupported = otherDevice.queueFamiliesSupported;                                                                               \
    physicalDevice = otherDevice.physicalDevice;                                                                                               \
    graphicsQueue = otherDevice.graphicsQueue;                                                                                                 \
    transferQueue = otherDevice.transferQueue;                                                                                                 \
    computeQueue = otherDevice.computeQueue;                                                                                                   \
    genericQueue = otherDevice.genericQueue;                                                                                                   \
    allQueues = otherDevice.allQueues;                                                                                                         \
    enabledFeatures = otherDevice.enabledFeatures;                                                                                             \
    logicalDevice = otherDevice.logicalDevice;                                                                                                 \
    timelineSemaphoreProps = otherDevice.timelineSemaphoreProps;                                                                               \
    timelineSemaphoreFeatures = otherDevice.timelineSemaphoreFeatures;                                                                         \
    memoryProperties = otherDevice.memoryProperties;                                                                                           \
    enabledDescIndexingFeatures = otherDevice.enabledDescIndexingFeatures;                                                                     \
    descIndexingFeatures = otherDevice.descIndexingFeatures;                                                                                   \
    descIndexingProps = otherDevice.descIndexingProps;                                                                                         \
    sync2Features = otherDevice.sync2Features;                                                                                                 \
    graphicsDebug = { this };

VulkanDevice::VulkanDevice(const VulkanDevice &otherDevice) { COPY_IMPL(); }

void VulkanDevice::operator=(const VulkanDevice &otherDevice) { COPY_IMPL(); }
#undef COPY_IMPL

VulkanDevice::~VulkanDevice()
{
    if (allQueues.size() > 0 || logicalDevice)
    {
        LOG_WARN("VulkanDevice", "Queues & logic devices not cleared");
        freeLogicDevice();
    }
}

template <typename QueueResType>
struct GetQueueCreateInfo
{
    VkDeviceQueueCreateInfo operator()(const QueueResType *queueRes)
    {
        CREATE_QUEUE_INFO(queueCreateInfo);
        queueRes->getQueueCreateInfo(queueCreateInfo);
        return queueCreateInfo;
    }
};

template <typename QueueResType>
struct CacheQueues
{
    void operator()(QueueResType *queueRes, VkDevice logicalDevice, PFN_vkGetDeviceQueue funcPtr)
    {
        queueRes->cacheQueues(logicalDevice, funcPtr);
    }
};

void VulkanDevice::createLogicDevice()
{
    LOG_DEBUG("VulkanDevice", "Creating logical device");
    const bool bQueueResCreated = createQueueResources();
    fatalAssertf(bQueueResCreated, "Without vulkan queues application cannot proceed running");
    markGlobalConstants();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(allQueues.size());

    std::set<int32> selectedQueueIndices; // Cannot request create for same queue family twice

    for (int32 queueFamIndex = 0; queueFamIndex < allQueues.size(); ++queueFamIndex)
    {
        VkDeviceQueueCreateInfo queueCreateInfo
            = VulkanQueueResourceInvoker::invoke<VkDeviceQueueCreateInfo, GetQueueCreateInfo>(allQueues[queueFamIndex]);

        if (selectedQueueIndices.insert(queueCreateInfo.queueFamilyIndex).second)
        {
            queueCreateInfos.push_back(queueCreateInfo);
        }
    }

    CREATE_DEVICE_INFO(deviceCreateInfo);
#if DEV_BUILD
    collectDeviceLayers(registeredLayers);
    deviceCreateInfo.enabledLayerCount = (uint32_t)registeredLayers.size();
    deviceCreateInfo.ppEnabledLayerNames = registeredLayers.data();
#endif
    const bool bExtsSupported = collectDeviceExtensions(registeredExtensions);
    fatalAssertf(bExtsSupported, "Failed collecting extensions");

    deviceCreateInfo.enabledExtensionCount = (uint32_t)registeredExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = registeredExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
    deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    // Additional features
    deviceCreateInfo.pNext = &timelineSemaphoreFeatures;
    timelineSemaphoreFeatures.pNext = &enabledDescIndexingFeatures;
    enabledDescIndexingFeatures.pNext = &sync2Features;

    const VkResult vulkanDeviceCreationResult = Vk::vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
    fatalAssertf(vulkanDeviceCreationResult == VK_SUCCESS, "Failed creating logical device");

    loadDeviceFunctions();

    for (QueueResourceBasePtr &queue : allQueues)
    {
        queue->init();
        VulkanQueueResourceInvoker::invoke<void, CacheQueues>(queue, logicalDevice, vkGetDeviceQueue);
    }

    GlobalRenderVariables::GPU_DEVICE_INITIALIZED.set(true);
}

void VulkanDevice::freeLogicDevice()
{
    LOG_DEBUG("VulkanDevice", "Freeing logical device");

    for (QueueResourceBasePtr &queueResPtr : allQueues)
    {
        if (queueResPtr)
        {
            queueResPtr->release();
            delete queueResPtr;
        }
    }
    allQueues.clear();
    graphicsQueue = nullptr;
    computeQueue = nullptr;
    transferQueue = nullptr;
    genericQueue = nullptr;

    vkDestroyDevice(logicalDevice, nullptr);
    logicalDevice = nullptr;
}

String VulkanDevice::getDeviceName() const { return UTF8_TO_TCHAR(properties.deviceName); }

VkPresentModeKHR VulkanDevice::getPresentMode() const { return globalPresentMode; }

VulkanDevice::QueueResourceBasePtr VulkanDevice::getGraphicsQueue() const { return graphicsQueue; }

VulkanDevice::QueueResourceBasePtr VulkanDevice::getComputeQueue() const { return computeQueue; }

VulkanDevice::QueueResourceBasePtr VulkanDevice::getTransferQueue() const { return transferQueue; }

VulkanDevice::QueueResourceBasePtr VulkanDevice::getGenericQueue() const { return genericQueue; }

int32 VulkanDevice::compare(const VulkanDevice &otherDevice, const WindowCanvasRef &windowCanvas) const
{
    if (windowCanvas.isValid())
    {
        int32 canvasChoice = compareSurfaceCompatibility(windowCanvas, otherDevice);
        if (canvasChoice != 0)
        {
            return canvasChoice;
        }
    }

    {
        int32 memoryChoice = compareMemoryCompatibility(otherDevice);
        if (memoryChoice != 0)
        {
            return memoryChoice;
        }
    }

    if (properties.deviceType != otherDevice.properties.deviceType)
    {
        int32 deviceTypeChoice = 0;
        switch (properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            deviceTypeChoice = -1;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            deviceTypeChoice = 1;
            break;
        default:
            break;
        }

        if (deviceTypeChoice == 0)
        {
            switch (otherDevice.properties.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                deviceTypeChoice = 1;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                deviceTypeChoice = -1;
                break;
            default:
                deviceTypeChoice = otherDevice.properties.deviceType - properties.deviceType;
                break;
            }
        }

        if (deviceTypeChoice != 0)
        {
            return deviceTypeChoice > 0 ? 1 : -1;
        }
    }

    // Todo(Jeslas) : decide between multiple same type  of cards here
    return 0;
}

bool VulkanDevice::isValidDevice() const { return physicalDevice != nullptr && queueFamiliesSupported.size() > 0; }

bool VulkanDevice::isLogicalDeviceCreated() const { return logicalDevice != nullptr; }

void VulkanDevice::getMemoryStat(uint64 &totalBudget, uint64 &usage, uint32 heapIndex)
{
    if (Vk::vkGetPhysicalDeviceMemoryProperties2KHR)
    {
        PHYSICAL_DEVICE_MEMORY_PROPERTIES_2(memProp);
        PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES(budget);
        memProp.pNext = &budget;
        Vk::vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice, &memProp);
        totalBudget = budget.heapBudget[heapIndex];
        usage = budget.heapUsage[heapIndex];
    }
    else
    {
        totalBudget = 0;
        usage = 0;
    }
}
