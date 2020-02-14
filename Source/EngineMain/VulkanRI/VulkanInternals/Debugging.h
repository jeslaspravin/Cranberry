#pragma once
#include <vulkan_core.h>
#include "../../Core/Platform/PlatformTypes.h"
#include <vec4.hpp>

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
		VkDebugUtilsMessageTypeFlagsEXT	messageTypes,const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	static VkBool32 vkDebugUtilsMessengerCallbackInfo(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT	messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	static VkBool32 vkDebugUtilsMessengerCallbackWarn(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT	messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	static VkBool32 vkDebugUtilsMessengerCallbackError(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT	messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
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
    void markObject(const IVulkanResources* resource) const;
    void markObject(const uint64& objectHandle, const String& objectName,VkObjectType objectType) const;
    void beginCmdBufferMarker(VkCommandBuffer commandBuffer, const String& name, const glm::vec4& color) const;
    void insertCmdBufferMarker(VkCommandBuffer commandBuffer, const String& name, const glm::vec4& color) const;
    void endCmdBufferMarker(VkCommandBuffer commandBuffer) const;
    void beginQueueMarker(VkQueue queue, const String& name, const glm::vec4& color) const;
    void insertQueueMarker(VkQueue queue, const String& name, const glm::vec4& color) const;
    void endQueueMarker(VkQueue queue) const;
};

struct ScopedCommandMarker
{
	VkCommandBuffer cmdBuffer;
	ScopedCommandMarker(VkCommandBuffer commandBuffer, const String& name, const glm::vec4& color);
	~ScopedCommandMarker();
};

struct ScopedQueueMarker
{
    VkQueue queue;
    ScopedQueueMarker(VkQueue q, const String& name, const glm::vec4& color);
    ~ScopedQueueMarker();
};