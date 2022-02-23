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


#include <iostream>
#include <algorithm>
#include <vulkan_core.h>
#include <sstream>
#include <set>

#include "VulkanGraphicsInstance.h"
#include "Types/Platform/PlatformFunctions.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "Modules/ModuleManager.h"
#include "Logger/Logger.h"
#include "VulkanInternals/VulkanMacros.h"
#include "RenderInterface/Resources/GraphicsResources.h"
#include "VulkanInternals/Debugging.h"
#include "VulkanInternals/VulkanMemoryAllocator.h"
#include "VulkanInternals/VulkanDescriptorAllocator.h"
#include "VulkanInternals/Commands/VulkanRenderCmdList.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "VulkanRHIModule.h"
#include "IApplicationModule.h"
#include "ApplicationInstance.h"

void VulkanGraphicsInstance::load()
{
    LOG_DEBUG("Vulkan", "%s() : Loading vulkan instance", __func__);
    loadGlobalFunctions();
    
    uint32 apiVersion;
    Vk::vkEnumerateInstanceVersion(&apiVersion);
    LOG_DEBUG("Vulkan", "%s() : Vulkan version %d.%d.%d", __func__, VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion)
        , VK_VERSION_PATCH(apiVersion));

    uint32 extensionCounts;
    VkResult result = Vk::vkEnumerateInstanceExtensionProperties(nullptr, (uint32_t*)&extensionCounts, nullptr);
    if (result != VkResult::VK_SUCCESS) {
        LOG_ERROR("Vulkan", "%s() : Failed to fetch extension properties",__func__);
    }

    availableInstanceExtensions.resize(extensionCounts);
    Vk::vkEnumerateInstanceExtensionProperties(nullptr, (uint32_t*)&extensionCounts, availableInstanceExtensions.data());
    LOG_DEBUG("Vulkan", "%s() : Fetched %d instance extension properties", __func__, extensionCounts);

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

    LOG_DEBUG("Vulkan", "%s() : Unloading vulkan instance", __func__);

    VulkanDebugLogger::unregisterDebugLogger();
    Vk::vkDestroyInstance(vulkanInstance, nullptr);
    vulkanInstance = nullptr;
}

void VulkanGraphicsInstance::loadGlobalFunctions()
{
#define EXPORTED_VK_FUNCTIONS(function) \
    Vk::##function=(PFN_##function) PlatformFunctions::getProcAddress(ModuleManager::get()->getOrLoadLibrary(TCHAR("vulkan-1.dll")), TCHAR(#function)); \
    if(Vk::##function == nullptr) \
    { \
        LOG_ERROR("Vulkan", "%s() : Loading failed for function :"#function,__func__);\
    }

#define  GLOBAL_VK_FUNCTIONS(function) \
    Vk::##function = (PFN_##function) Vk::vkGetInstanceProcAddr(nullptr,#function); \
    if(Vk::##function == nullptr)\
    { \
        LOG_ERROR("Vulkan", "%s() : Loading failed for global function :"#function,__func__);\
    }

#include "VulkanInternals/VulkanFunctionLists.inl"


}

void VulkanGraphicsInstance::createVulkanInstance()
{
    LOG_DEBUG("Vulkan", "%s() : Creating vulkan application instance", __func__);
    CREATE_APP_INFO(appInfo);
    const ApplicationInstance* appInstance = IApplicationModule::get()->getApplication();
    appInfo.pApplicationName = (const AChar*)TCHAR_TO_UTF8(appInstance->getAppName().getChar());
    int32 majorVer, minorVer, headVer;
    appInstance->getVersion(headVer, majorVer, minorVer);

    appInfo.applicationVersion = VK_MAKE_VERSION(headVer, majorVer, minorVer);
    appInfo.pEngineName = MACRO_TO_STRING(ENGINE_NAME);
    appInfo.engineVersion = VK_MAKE_VERSION(headVer, majorVer, minorVer);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);

    CREATE_INSTANCE_INFO(instanceCreateInfo);
    std::vector<std::string::const_pointer> layers;
#if DEV_BUILD
    collectInstanceLayers(layers);
    instanceCreateInfo.ppEnabledLayerNames = layers.size() > 0 ? &layers[0] : nullptr;
    instanceCreateInfo.enabledLayerCount = (uint32_t)layers.size();
#endif

    if (!collectInstanceExtensions(registeredInstanceExtensions))
    {
        LOG_ERROR("Vulkan", "%s() : Failed collecting extensions", __func__);
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
        LOG_ERROR("Vulkan", "%s() : Requested layer/s not available%s", __func__, layersStr.getChar());
    }
    fatalAssert(result == VkResult::VK_SUCCESS && vulkanInstance != nullptr,"Could not create vulkan instance");
}

#if DEV_BUILD
void VulkanGraphicsInstance::collectInstanceLayers(std::vector<const char*>& layers) const
{
    layers.push_back("VK_LAYER_KHRONOS_validation");
}
#endif

bool VulkanGraphicsInstance::collectInstanceExtensions(std::vector<const char*>& extensions) const
{
    extensions.clear();

    std::set<std::string::const_pointer> mandatoryExtensions;

#define INSTANCE_VK_EXT_FUNCTIONS(function,extension) mandatoryExtensions.insert(extension);
#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function) mandatoryExtensions.insert( PFN_##function::EXT_NAME );

#include "VulkanInternals/VulkanFunctionLists.inl"

    std::stringstream extensionsStream;
    for (const VkExtensionProperties& extProperty:availableInstanceExtensions) {
        extensionsStream << extProperty.extensionName;
    }
    std::string extensionsString = extensionsStream.str();

    for (const auto* mandatoryExt : mandatoryExtensions) {
        if(extensionsString.find(mandatoryExt,0) != String::npos)
        {
            extensions.push_back(mandatoryExt);
            LOG_DEBUG("Vulkan", "%s() : Loading instance extension %s", __func__, mandatoryExt);
        }
    }

    if (mandatoryExtensions.size() != extensions.size()) {
        LOG_ERROR("Vulkan", "%s() : Missing mandatory extensions",__func__);
        return false;
    }
    
    return true;
}

void VulkanGraphicsInstance::loadInstanceFunctions()
{
// TODO(Jeslas) : Find better way if possible, Find a way to push these extension automatically into collect instance's mandatory extensions
#define INSTANCE_VK_FUNCTIONS(function)\
    Vk::##function = (PFN_##function)Vk::vkGetInstanceProcAddr(vulkanInstance,#function);\
    if(Vk::##function == nullptr) LOG_ERROR("Vulkan","%s() : Failed loading function : "#function,__func__);

#define INSTANCE_VK_EXT_FUNCTIONS(function,extension)\
    for(const char* ext:registeredInstanceExtensions) \
    {\
        if (std::strcmp(ext, extension) == 0)\
        {\
            Vk::##function = (PFN_##function)Vk::vkGetInstanceProcAddr(vulkanInstance, #function); \
            break;\
        }\
    } \
    if (Vk::##function == nullptr) LOG_ERROR("Vulkan", "%s() : Failed loading function : "#function, __func__);

#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function)\
    for(const char* ext:registeredInstanceExtensions) \
    {\
        if (std::strcmp(ext, PFN_##function::EXT_NAME ) == 0)\
        {\
            Vk::##function = PFN_##function(); \
            break;\
        }\
    }    

#include "VulkanInternals/VulkanFunctionLists.inl"
}

void VulkanGraphicsInstance::createVulkanDevice(const WindowCanvasRef& windowCanvas)
{
    uint32 numPhysicalDevices;
    
    if (Vk::vkEnumeratePhysicalDevices(vulkanInstance, (uint32_t*)&numPhysicalDevices, nullptr) != VK_SUCCESS) 
    {
        LOG_ERROR("Vulkan", "%s() : Enumerating physical device failed! no graphics device found", __func__);
        return;
    }
    std::vector<VkPhysicalDevice> vulkanPhysicalDevices;
    vulkanPhysicalDevices.resize(numPhysicalDevices);
    Vk::vkEnumeratePhysicalDevices(vulkanInstance, (uint32_t*)&numPhysicalDevices, vulkanPhysicalDevices.data());

    std::vector<VulkanDevice> vulkanDevices;
    vulkanDevices.reserve(numPhysicalDevices);

    for (VkPhysicalDevice& device : vulkanPhysicalDevices)
    {
        VulkanDevice vulkanDevice = VulkanDevice(std::move(device));
        if(vulkanDevice.isValidDevice()) vulkanDevices.push_back(vulkanDevice);
    }

    std::sort(vulkanDevices.begin(), vulkanDevices.end(), VulkanDeviceCompare{ windowCanvas });
    selectedDevice = std::move(vulkanDevices[0]);
    vulkanDevices.clear();

    LOG_DEBUG("Vulkan", "%s() : Selected device %s", __func__, selectedDevice.getDeviceName().getChar());
}

void VulkanGraphicsInstance::updateSurfaceDependents()
{
    // We have to create device after surface creation since device queue needs surface to select present queue
    // Once we have none present application support we can redo this order based on presenting or not
    if (!selectedDevice.isValidDevice())
    {
        createVulkanDevice(nullptr);
        fatalAssert(selectedDevice.isValidDevice(), "Graphics device creation failed");
    }
    if (!selectedDevice.isLogicalDeviceCreated())
    {
        selectedDevice.createLogicDevice();
        memoryAllocator = IVulkanMemoryAllocator::createAllocator(&selectedDevice);
        descriptorsSetAllocator = SharedPtr<VulkanDescriptorsSetAllocator>(new VulkanDescriptorsSetAllocator(&selectedDevice));
        vulkanCmdList = SharedPtr<VulkanCommandList>(new VulkanCommandList(this, IVulkanRHIModule::get()->getGraphicsHelper(), &selectedDevice));
    }
    selectedDevice.cacheGlobalSurfaceProperties(nullptr);
}

void VulkanGraphicsInstance::initializeCmds(class IRenderCommandList* commandList)
{
    commandList->setup(vulkanCmdList.get());
}

