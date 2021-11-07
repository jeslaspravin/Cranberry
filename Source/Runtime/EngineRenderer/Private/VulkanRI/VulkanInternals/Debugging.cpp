#include "VulkanRI/VulkanInternals/Debugging.h"
#include "String/String.h"
#include "Logger/Logger.h"
#include "VulkanRI/VulkanInternals/VulkanFunctions.h"
#include "VulkanRI/VulkanInternals/VulkanMacros.h"
#include "VulkanRI/Resources/IVulkanResources.h"
#include "VulkanRI/VulkanGraphicsHelper.h"
#include "Engine/GameEngine.h"
#include "VulkanRI/VulkanInternals/VulkanDevice.h"
#include "Types/Platform/PlatformAssertionErrors.h"

struct DebugMessengerData
{
    VkInstance vulkanInstance = nullptr;
    VkDebugUtilsMessengerEXT debugMsgrPtr = nullptr;
    VkDebugUtilsMessengerEXT infoMsgrPtr = nullptr;
    VkDebugUtilsMessengerEXT warnMsgrPtr = nullptr;
    VkDebugUtilsMessengerEXT errorMsgrPtr = nullptr;
};

DebugMessengerData& VulkanDebugLogger::getData()
{
    static DebugMessengerData data;
    return data;
}

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackDebug(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    String message = messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        ? "[General]" : messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        ? "[Performance]" : "[Validation]";
    message.append("[ID : %d] [Name : %s] Message : %s");

    Logger::debug("VulkanDebugUtils", message, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, 
        pCallbackData->pMessage);

    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        Logger::debug("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            Logger::debug("VulkanDebugUtils", "        %d : %s", i, 
                pCallbackData->pQueueLabels[i].pLabelName == nullptr?"NullName": pCallbackData->pQueueLabels[i].pLabelName);
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        Logger::debug("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            Logger::debug("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? "NullName" : pCallbackData->pCmdBufLabels[i].pLabelName);
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        Logger::debug("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            Logger::debug("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? "NullName" : pCallbackData->pObjects[i].pObjectName);
        }
    }

    return VK_FALSE;
}

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackInfo(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{

    String message = messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        ? "[General]" : messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        ? "[Performance]" : "[Validation]";
    message.append("[ID : %d] [Name : %s] Message : %s");

    Logger::log("VulkanDebugUtils", message, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName,
        pCallbackData->pMessage);

    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        Logger::log("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            Logger::log("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pQueueLabels[i].pLabelName == nullptr ? "NullName" : pCallbackData->pQueueLabels[i].pLabelName);
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        Logger::log("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            Logger::log("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? "NullName" : pCallbackData->pCmdBufLabels[i].pLabelName);
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        Logger::log("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            Logger::log("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? "NullName" : pCallbackData->pObjects[i].pObjectName);
        }
    }

    return VK_FALSE;
}

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackWarn(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    String message = messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        ? "[General]" : messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        ? "[Performance]" : "[Validation]";
    message.append("[ID : %d] [Name : %s] Message : %s");

    Logger::warn("VulkanDebugUtils", message, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName,
        pCallbackData->pMessage);
    
    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        Logger::warn("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            Logger::warn("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pQueueLabels[i].pLabelName == nullptr ? "NullName" : pCallbackData->pQueueLabels[i].pLabelName);
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        Logger::warn("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            Logger::warn("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? "NullName" : pCallbackData->pCmdBufLabels[i].pLabelName);
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        Logger::warn("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            Logger::warn("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? "NullName" : pCallbackData->pObjects[i].pObjectName);
        }
    }

    return VK_FALSE;
}

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackError(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    String message = messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        ? "[General]" : messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        ? "[Performance]" : "[Validation]";
    message.append("[ID : %d] [Name : %s] Message : %s");

    Logger::error("VulkanDebugUtils", message, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName,
        pCallbackData->pMessage);
    
    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        Logger::error("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            Logger::error("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pQueueLabels[i].pLabelName == nullptr ? "NullName" : pCallbackData->pQueueLabels[i].pLabelName);
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        Logger::error("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            Logger::error("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? "NullName" : pCallbackData->pCmdBufLabels[i].pLabelName);
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        Logger::error("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            Logger::error("VulkanDebugUtils", "        %d : %s", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? "NullName" : pCallbackData->pObjects[i].pObjectName);
        }
    }


    debugAssert(!"Vulkan Error!");
    return VK_FALSE;
}

bool VulkanDebugLogger::registerDebugLogger(const VkInstance vulkanInstance)
{
#if _DEBUG & _VERBOSE
    {
        CREATE_DEBUG_UTILS_MESSENGER_INFO(debugCreateInfo);
        debugCreateInfo.messageSeverity = VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        debugCreateInfo.messageType = VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        debugCreateInfo.pfnUserCallback = &VulkanDebugLogger::vkDebugUtilsMessengerCallbackDebug;

        Vk::vkCreateDebugUtilsMessengerEXT(vulkanInstance, &debugCreateInfo, nullptr, &getData().debugMsgrPtr);

        debugCreateInfo.messageSeverity = VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        debugCreateInfo.pfnUserCallback = &VulkanDebugLogger::vkDebugUtilsMessengerCallbackInfo;
        Vk::vkCreateDebugUtilsMessengerEXT(vulkanInstance, &debugCreateInfo, nullptr, &getData().infoMsgrPtr);
    }
#endif
#if _DEBUG
    {
        CREATE_DEBUG_UTILS_MESSENGER_INFO(debugCreateInfo);
        debugCreateInfo.messageSeverity = VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debugCreateInfo.messageType = VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        debugCreateInfo.pfnUserCallback = &VulkanDebugLogger::vkDebugUtilsMessengerCallbackWarn;

        Vk::vkCreateDebugUtilsMessengerEXT(vulkanInstance, &debugCreateInfo, nullptr, &getData().warnMsgrPtr);

        debugCreateInfo.messageSeverity = VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.pfnUserCallback = &VulkanDebugLogger::vkDebugUtilsMessengerCallbackError;
        Vk::vkCreateDebugUtilsMessengerEXT(vulkanInstance, &debugCreateInfo, nullptr, &getData().errorMsgrPtr);
    }
#endif

    getData().vulkanInstance = vulkanInstance;
    return true;
}

void VulkanDebugLogger::unregisterDebugLogger()
{
#if _DEBUG
    if (!getData().vulkanInstance)
        return;

    if (getData().debugMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().debugMsgrPtr,nullptr);
    }
    if (getData().infoMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().infoMsgrPtr,nullptr);
    }
    if (getData().warnMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().warnMsgrPtr, nullptr);
    }
    if (getData().errorMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().errorMsgrPtr,nullptr);
    }
#endif
}

VulkanDebugGraphics::VulkanDebugGraphics(VulkanDevice* device): ownerDevice(device)
{}

VulkanDebugGraphics::VulkanDebugGraphics(const VulkanDebugGraphics& other) : ownerDevice(other.ownerDevice)
{}

VulkanDebugGraphics::VulkanDebugGraphics(VulkanDebugGraphics&& rValue) : ownerDevice(std::move(rValue.ownerDevice))
{}

void VulkanDebugGraphics::operator=(const VulkanDebugGraphics& other)
{
    ownerDevice = other.ownerDevice;
}

void VulkanDebugGraphics::operator=(VulkanDebugGraphics&& rValue)
{
    ownerDevice = std::move(rValue.ownerDevice);
}

void VulkanDebugGraphics::markObject(const IVulkanResources* resource) const
{
    if (!resource || resource->getDispatchableHandle() == 0)
        return;

    DEBUG_UTILS_OBJECT_NAME_INFO(objectNameInfo);
    objectNameInfo.objectHandle = resource->getDispatchableHandle();
    objectNameInfo.objectType = resource->getObjectType();
    String name = resource->getObjectName();
    objectNameInfo.pObjectName = name.getChar();
    
    ownerDevice->vkSetDebugUtilsObjectNameEXT(VulkanGraphicsHelper::getDevice(ownerDevice), &objectNameInfo);
}


void VulkanDebugGraphics::markObject(const uint64& objectHandle, const String& objectName, VkObjectType objectType) const
{
    if (objectHandle == 0)
        return;

    DEBUG_UTILS_OBJECT_NAME_INFO(objectNameInfo);
    objectNameInfo.objectHandle = objectHandle;
    objectNameInfo.objectType = objectType;
    objectNameInfo.pObjectName = objectName.getChar();

    ownerDevice->vkSetDebugUtilsObjectNameEXT(VulkanGraphicsHelper::getDevice(ownerDevice), &objectNameInfo);
}

void VulkanDebugGraphics::beginCmdBufferMarker(VkCommandBuffer commandBuffer, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    label.pLabelName = name.getChar();

    ownerDevice->vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
}

void VulkanDebugGraphics::insertCmdBufferMarker(VkCommandBuffer commandBuffer, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    label.pLabelName = name.getChar();

    ownerDevice->vkCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
}

void VulkanDebugGraphics::endCmdBufferMarker(VkCommandBuffer commandBuffer) const
{
    ownerDevice->vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

void VulkanDebugGraphics::beginQueueMarker(VkQueue queue, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    label.pLabelName = name.getChar();

    ownerDevice->vkQueueBeginDebugUtilsLabelEXT(queue, &label);
}

void VulkanDebugGraphics::insertQueueMarker(VkQueue queue, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    label.pLabelName = name.getChar();

    ownerDevice->vkQueueInsertDebugUtilsLabelEXT(queue, &label);
}

void VulkanDebugGraphics::endQueueMarker(VkQueue queue) const
{
    ownerDevice->vkQueueEndDebugUtilsLabelEXT(queue);
}

ScopedVulkanCommandMarker::ScopedVulkanCommandMarker(VkCommandBuffer commandBuffer, const String& name, const LinearColor& color /* = LinearColorConst::WHITE*/)
{
    cmdBuffer = commandBuffer;
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(gEngine->getRenderManager()->getGraphicsInstance());
    graphicsDebugger->beginCmdBufferMarker(cmdBuffer, name, color);
}

ScopedVulkanCommandMarker::~ScopedVulkanCommandMarker()
{
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(gEngine->getRenderManager()->getGraphicsInstance());
    graphicsDebugger->endCmdBufferMarker(cmdBuffer);
}

ScopedVulkanQueueMarker::ScopedVulkanQueueMarker(VkQueue q, const String& name, const LinearColor& color /* = LinearColorConst::WHITE*/)
{
    queue = q;
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(gEngine->getRenderManager()->getGraphicsInstance());
    graphicsDebugger->beginQueueMarker(queue, name, color);
}

ScopedVulkanQueueMarker::~ScopedVulkanQueueMarker()
{
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(gEngine->getRenderManager()->getGraphicsInstance());
    graphicsDebugger->endQueueMarker(queue);
}
