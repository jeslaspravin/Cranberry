/*!
 * \file VulkanGraphicsInstance.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <vulkan_core.h>

#include "ApplicationInstance.h"
#include "IApplicationModule.h"
#include "Logger/Logger.h"
#include "Modules/ModuleManager.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Resources/GraphicsResources.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "VulkanGraphicsInstance.h"
#include "VulkanInternals/Commands/VulkanRenderCmdList.h"
#include "VulkanInternals/Debugging.h"
#include "VulkanInternals/VulkanDescriptorAllocator.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "VulkanInternals/VulkanMacros.h"
#include "VulkanInternals/VulkanMemoryAllocator.h"
#include "VulkanRHIModule.h"

void VulkanGraphicsInstance::load()
{
    LOG_DEBUG("Vulkan", "Loading vulkan instance");
    loadGlobalFunctions();

    uint32 apiVersion;
    Vk::vkEnumerateInstanceVersion(&apiVersion);
    const uint32 hVersion = VK_HEADER_VERSION_COMPLETE;
    LOG_DEBUG("Vulkan", "Header version {}.{}.{}", VK_VERSION_MAJOR(hVersion), VK_VERSION_MINOR(hVersion), VK_VERSION_PATCH(hVersion));
    LOG_DEBUG("Vulkan", "API version {}.{}.{}", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));

    uint32 extensionCounts;
    VkResult result = Vk::vkEnumerateInstanceExtensionProperties(nullptr, (uint32_t *)&extensionCounts, nullptr);
    if (result != VkResult::VK_SUCCESS)
    {
        LOG_ERROR("Vulkan", "Failed to fetch extension properties");
    }

    availableInstanceExtensions.resize(extensionCounts);
    Vk::vkEnumerateInstanceExtensionProperties(nullptr, (uint32_t *)&extensionCounts, availableInstanceExtensions.data());
    LOG_DEBUG("Vulkan", "Fetched {} instance extension properties", extensionCounts);

    createVulkanInstance();
    loadInstanceFunctions();

    VulkanDebugLogger::registerDebugLogger(vulkanInstance);
}

void VulkanGraphicsInstance::unload()
{
    if (selectedDevice.isValidDevice())
    {
        memoryAllocator.reset();
        descriptorsSetAllocator.reset();
        vulkanCmdList.reset();
        selectedDevice.freeLogicDevice();
    }

    LOG_DEBUG("Vulkan", "Unloading vulkan instance");

    VulkanDebugLogger::unregisterDebugLogger();
    Vk::vkDestroyInstance(vulkanInstance, nullptr);
    vulkanInstance = nullptr;
}

void VulkanGraphicsInstance::loadGlobalFunctions()
{
#define EXPORTED_VK_FUNCTIONS(function)                                                                                                        \
    Vk::##function                                                                                                                             \
        = (PFN_##function)PlatformFunctions::getProcAddress(ModuleManager::get()->getOrLoadLibrary(TCHAR("vulkan-1.dll")), TCHAR(#function));  \
    if (Vk::##function == nullptr)                                                                                                             \
    {                                                                                                                                          \
        LOG_ERROR("Vulkan", "Loading failed for function :" #function);                                                                        \
    }

#define GLOBAL_VK_FUNCTIONS(function)                                                                                                          \
    Vk::##function = (PFN_##function)Vk::vkGetInstanceProcAddr(nullptr, #function);                                                            \
    if (Vk::##function == nullptr)                                                                                                             \
    {                                                                                                                                          \
        LOG_ERROR("Vulkan", "Loading failed for global function :" #function);                                                                 \
    }

#include "VulkanInternals/VulkanFunctionLists.inl"
}

void VulkanGraphicsInstance::createVulkanInstance()
{
    LOG_DEBUG("Vulkan", "Creating vulkan application instance");
    CREATE_APP_INFO(appInfo);
    const ApplicationInstance *appInstance = IApplicationModule::get()->getApplication();
    appInfo.pApplicationName = (const AChar *)TCHAR_TO_UTF8(appInstance->getAppName().getChar());
    int32 majorVer, minorVer, headVer;
    appInstance->getVersion(headVer, majorVer, minorVer);

    appInfo.applicationVersion = VK_MAKE_VERSION(headVer, majorVer, minorVer);
    appInfo.pEngineName = MACRO_TO_STRING(ENGINE_NAME);
    appInfo.engineVersion = VK_MAKE_VERSION(headVer, majorVer, minorVer);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);

    CREATE_INSTANCE_INFO(instanceCreateInfo);
    std::vector<std::string::const_pointer> layers;
#if DEV_BUILD
    collectInstanceLayers(layers);
    instanceCreateInfo.ppEnabledLayerNames = layers.size() > 0 ? &layers[0] : nullptr;
    instanceCreateInfo.enabledLayerCount = (uint32_t)layers.size();
#endif

    if (!collectInstanceExtensions(registeredInstanceExtensions))
    {
        LOG_ERROR("Vulkan", "Failed collecting extensions");
        debugAssert(!"Necessary extensions are not collected!");
    }

    instanceCreateInfo.ppEnabledExtensionNames = &registeredInstanceExtensions[0];
    instanceCreateInfo.enabledExtensionCount = (uint32_t)registeredInstanceExtensions.size();
    instanceCreateInfo.pApplicationInfo = &appInfo;

    VkResult result = Vk::vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanInstance);

    if (result == VkResult::VK_ERROR_LAYER_NOT_PRESENT)
    {
        String layersStr;
        for (std::string::const_pointer layer : layers)
        {
            layersStr.append(TCHAR("\n\t"));
            layersStr.append(UTF8_TO_TCHAR(layer));
        }
        LOG_ERROR("Vulkan", "Requested layer/s not available{}", layersStr.getChar());
    }
    fatalAssertf(result == VkResult::VK_SUCCESS && vulkanInstance != nullptr, "Could not create vulkan instance");
}

#if DEV_BUILD
void VulkanGraphicsInstance::collectInstanceLayers(std::vector<const char *> &layers) const { layers.push_back("VK_LAYER_KHRONOS_validation"); }
#endif

bool VulkanGraphicsInstance::collectInstanceExtensions(std::vector<const char *> &extensions) const
{
    extensions.clear();

    std::set<std::string::const_pointer> mandatoryExtensions;

#define INSTANCE_VK_EXT_FUNCTIONS(function, extension) mandatoryExtensions.insert(extension);
#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function) mandatoryExtensions.insert(PFN_##function::EXT_NAME);

#include "VulkanInternals/VulkanFunctionLists.inl"

    std::stringstream extensionsStream;
    for (const VkExtensionProperties &extProperty : availableInstanceExtensions)
    {
        extensionsStream << extProperty.extensionName;
    }
    std::string extensionsString = extensionsStream.str();

    for (const auto *mandatoryExt : mandatoryExtensions)
    {
        if (extensionsString.find(mandatoryExt, 0) != String::npos)
        {
            extensions.push_back(mandatoryExt);
            LOG_DEBUG("Vulkan", "Loading instance extension {}", mandatoryExt);
        }
    }

    if (mandatoryExtensions.size() != extensions.size())
    {
        LOG_ERROR("Vulkan", "Missing mandatory extensions");
        return false;
    }

    return true;
}

void VulkanGraphicsInstance::loadInstanceFunctions()
{
// TODO(Jeslas) : Find better way if possible, Find a way to push these extension automatically into
// collect instance's mandatory extensions
#define INSTANCE_VK_FUNCTIONS(function)                                                                                                        \
    Vk::##function = (PFN_##function)Vk::vkGetInstanceProcAddr(vulkanInstance, #function);                                                     \
    if (Vk::##function == nullptr)                                                                                                             \
        LOG_ERROR("Vulkan", "Failed loading function : " #function);

#define INSTANCE_VK_EXT_FUNCTIONS(function, extension)                                                                                         \
    for (const char *ext : registeredInstanceExtensions)                                                                                       \
    {                                                                                                                                          \
        if (std::strcmp(ext, extension) == 0)                                                                                                  \
        {                                                                                                                                      \
            Vk::##function = (PFN_##function)Vk::vkGetInstanceProcAddr(vulkanInstance, #function);                                             \
            break;                                                                                                                             \
        }                                                                                                                                      \
    }                                                                                                                                          \
    if (Vk::##function == nullptr)                                                                                                             \
        LOG_ERROR("Vulkan", "Failed loading function : " #function);

#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function)                                                                                           \
    for (const char *ext : registeredInstanceExtensions)                                                                                       \
    {                                                                                                                                          \
        if (std::strcmp(ext, PFN_##function::EXT_NAME) == 0)                                                                                   \
        {                                                                                                                                      \
            Vk::##function = PFN_##function();                                                                                                 \
            break;                                                                                                                             \
        }                                                                                                                                      \
    }

#include "VulkanInternals/VulkanFunctionLists.inl"
}

void VulkanGraphicsInstance::createVulkanDevice(const WindowCanvasRef &windowCanvas)
{
    uint32 numPhysicalDevices;

    if (Vk::vkEnumeratePhysicalDevices(vulkanInstance, (uint32_t *)&numPhysicalDevices, nullptr) != VK_SUCCESS)
    {
        LOG_ERROR("Vulkan", "Enumerating physical device failed! no graphics device found");
        return;
    }
    std::vector<VkPhysicalDevice> vulkanPhysicalDevices;
    vulkanPhysicalDevices.resize(numPhysicalDevices);
    Vk::vkEnumeratePhysicalDevices(vulkanInstance, (uint32_t *)&numPhysicalDevices, vulkanPhysicalDevices.data());

    std::vector<VulkanDevice> vulkanDevices;
    vulkanDevices.reserve(numPhysicalDevices);

    for (VkPhysicalDevice &device : vulkanPhysicalDevices)
    {
        VulkanDevice vulkanDevice = VulkanDevice(std::move(device));
        if (vulkanDevice.isValidDevice())
        {
            vulkanDevices.push_back(vulkanDevice);
        }
    }

    std::sort(vulkanDevices.begin(), vulkanDevices.end(), VulkanDeviceCompare{ windowCanvas });
    selectedDevice = std::move(vulkanDevices[0]);
    vulkanDevices.clear();

    LOG_DEBUG("Vulkan", "Selected device {}", selectedDevice.getDeviceName().getChar());
}

void VulkanGraphicsInstance::updateSurfaceDependents()
{
    // We have to create device after surface creation since device queue needs surface to select present
    // queue Once we have none present application support we can redo this order based on presenting or
    // not
    if (!selectedDevice.isValidDevice())
    {
        createVulkanDevice(nullptr);
        fatalAssertf(selectedDevice.isValidDevice(), "Graphics device creation failed");
    }
    if (!selectedDevice.isLogicalDeviceCreated())
    {
        selectedDevice.createLogicDevice();
        memoryAllocator = IVulkanMemoryAllocator::createAllocator(&selectedDevice);
        descriptorsSetAllocator = SharedPtr<VulkanDescriptorsSetAllocator>(new VulkanDescriptorsSetAllocator(&selectedDevice));
        vulkanCmdList
            = SharedPtr<VulkanCommandList>(new VulkanCommandList(this, IVulkanRHIModule::get()->getGraphicsHelper(), &selectedDevice));
    }
    selectedDevice.cacheGlobalSurfaceProperties(nullptr);
}

void VulkanGraphicsInstance::initializeCmds(class IRenderCommandList *commandList) { commandList->setup(vulkanCmdList.get()); }
