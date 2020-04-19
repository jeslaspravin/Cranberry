#pragma once
#include "../../Core/Platform/PlatformTypes.h"
#include "../../Core/Types/Colors.h"
#include <vulkan_core.h>
#include <glm/vec4.hpp>

struct DebugMessengerData;
class IVulkanResources;
class String;
class VulkanDevice;

class VulkanDebugLogger
{
private:
    VulkanDebugLogger(){}

    static DebugMessengerData& getData();

    static VkBool32 vkDebugUtilsMessengerCallbackDebug(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT    messageTypes,const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    static VkBool32 vkDebugUtilsMessengerCallbackInfo(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT    messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    static VkBool32 vkDebugUtilsMessengerCallbackWarn(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT    messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    static VkBool32 vkDebugUtilsMessengerCallbackError(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT    messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

public:
    static bool registerDebugLogger(const VkInstance vulkanInstance);
    static void unregisterDebugLogger();
};

class VulkanDebugGraphics
{
private:
    const VulkanDevice* ownerDevice;

public:
    VulkanDebugGraphics() : ownerDevice(nullptr){}
    VulkanDebugGraphics(VulkanDevice* device);

    VulkanDebugGraphics(const VulkanDebugGraphics& other);
    VulkanDebugGraphics(VulkanDebugGraphics&& rValue);
    void operator=(const VulkanDebugGraphics& other);
    void operator=(VulkanDebugGraphics&& rValue);

    void markObject(const IVulkanResources* resource) const;
    void markObject(const uint64& objectHandle, const String& objectName,VkObjectType objectType) const;
    void beginCmdBufferMarker(VkCommandBuffer commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const;
    void insertCmdBufferMarker(VkCommandBuffer commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const;
    void endCmdBufferMarker(VkCommandBuffer commandBuffer) const;
    void beginQueueMarker(VkQueue queue, const String& name, const LinearColor& color = LinearColorConst::WHITE) const;
    void insertQueueMarker(VkQueue queue, const String& name, const LinearColor& color = LinearColorConst::WHITE) const;
    void endQueueMarker(VkQueue queue) const;
};

struct ScopedCommandMarker
{
    VkCommandBuffer cmdBuffer;
    ScopedCommandMarker(VkCommandBuffer commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE);
    ~ScopedCommandMarker();
};

struct ScopedQueueMarker
{
    VkQueue queue;
    ScopedQueueMarker(VkQueue q, const String& name, const LinearColor& color = LinearColorConst::WHITE);
    ~ScopedQueueMarker();
};