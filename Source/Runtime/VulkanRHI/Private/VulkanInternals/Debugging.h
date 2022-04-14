/*!
 * \file Debugging.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/Colors.h"
#include "Types/CoreTypes.h"

#include <vulkan_core.h>

struct DebugMessengerData;
class IVulkanResources;
class String;
class VulkanDevice;

class VulkanDebugLogger
{
private:
    VulkanDebugLogger() {}

    static DebugMessengerData &getData();

    static String messageTypeStr(VkDebugUtilsMessageTypeFlagsEXT messageTypes);

    static VkBool32 vkDebugUtilsMessengerCallbackDebug(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

    static VkBool32 vkDebugUtilsMessengerCallbackInfo(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

    static VkBool32 vkDebugUtilsMessengerCallbackWarn(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

    static VkBool32 vkDebugUtilsMessengerCallbackError(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

public:
    static bool registerDebugLogger(const VkInstance vulkanInstance);
    static void unregisterDebugLogger();
};

class VulkanDebugGraphics
{
private:
    const VulkanDevice *ownerDevice;

public:
    VulkanDebugGraphics()
        : ownerDevice(nullptr)
    {}
    VulkanDebugGraphics(VulkanDevice *device);

    VulkanDebugGraphics(const VulkanDebugGraphics &other);
    VulkanDebugGraphics(VulkanDebugGraphics &&rValue);
    void operator=(const VulkanDebugGraphics &other);
    void operator=(VulkanDebugGraphics &&rValue);

    void markObject(const IVulkanResources *resource) const;
    void markObject(const uint64 &objectHandle, const String &objectName, VkObjectType objectType) const;
    void beginCmdBufferMarker(VkCommandBuffer commandBuffer, const String &name,
        const LinearColor &color = LinearColorConst::WHITE) const;
    void insertCmdBufferMarker(VkCommandBuffer commandBuffer, const String &name,
        const LinearColor &color = LinearColorConst::WHITE) const;
    void endCmdBufferMarker(VkCommandBuffer commandBuffer) const;
    void beginQueueMarker(
        VkQueue queue, const String &name, const LinearColor &color = LinearColorConst::WHITE) const;
    void insertQueueMarker(
        VkQueue queue, const String &name, const LinearColor &color = LinearColorConst::WHITE) const;
    void endQueueMarker(VkQueue queue) const;
};

#define SCOPED_VULKAN_CMD_MARKER(CommandBuffer, Name)                                                   \
    ScopedVulkanCommandMarker cmdMarker_##Name(CommandBuffer, #Name)
#define SCOPED_VULKAN_CMD_COLORMARKER(CommandBuffer, Name, Color)                                       \
    ScopedVulkanCommandMarker cmdMarker_##Name(CommandBuffer, #Name, Color)
struct ScopedVulkanCommandMarker
{
    VkCommandBuffer cmdBuffer;
    ScopedVulkanCommandMarker(VkCommandBuffer commandBuffer, const String &name,
        const LinearColor &color = LinearColorConst::WHITE);
    ~ScopedVulkanCommandMarker();
};

#define SCOPED_VULKAN_QUEUE_MARKER(Queue, Name) ScopedVulkanQueueMarker queueMarker_##Name(Queue, #Name)
#define SCOPED_VULKAN_QUEUE_COLORMARKER(Queue, Name, Color)                                             \
    ScopedVulkanQueueMarker queueMarker_##Name(Queue, #Name, Color)
struct ScopedVulkanQueueMarker
{
    VkQueue queue;
    ScopedVulkanQueueMarker(
        VkQueue q, const String &name, const LinearColor &color = LinearColorConst::WHITE);
    ~ScopedVulkanQueueMarker();
};