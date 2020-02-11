#pragma once
#include "vulkan_core.h"

struct DebugMessengerData;

class VulkanDebugLogger
{
private:
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