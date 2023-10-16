/*!
 * \file Debugging.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/Debugging.h"
#include "Logger/Logger.h"
#include "Resources/IVulkanResources.h"
#include "String/String.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanFunctions.h"
#include "VulkanInternals/VulkanMacros.h"
#include "VulkanRHIModule.h"

struct DebugMessengerData
{
    VkInstance vulkanInstance = nullptr;
    VkDebugUtilsMessengerEXT debugMsgrPtr = nullptr;
    VkDebugUtilsMessengerEXT infoMsgrPtr = nullptr;
    VkDebugUtilsMessengerEXT warnMsgrPtr = nullptr;
    VkDebugUtilsMessengerEXT errorMsgrPtr = nullptr;
};

DebugMessengerData &VulkanDebugLogger::getData()
{
    static DebugMessengerData data;
    return data;
}

const TChar *VulkanDebugLogger::messageTypeStr(VkDebugUtilsMessageTypeFlagsEXT messageTypes)
{
    const TChar *messageType = messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        ? TCHAR("[General]")
        : messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        ? TCHAR("[Performance]")
        : TCHAR("[Validation]");
    return messageType;
}
constexpr static const TChar *NULL_MSG_ID = TCHAR("NullMsgID");
constexpr static const TChar *NULL_MSG = TCHAR("NullMsg");
constexpr static const TChar *NULL_LABEL = TCHAR("NullLabel");
constexpr static const TChar *NULL_OBJ_NAME = TCHAR("NullObjName");

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackDebug(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * /*pUserData*/
)
{
    LOG_DEBUG(
        "VulkanDebugUtils", "{}[ID : {}][Name : {}] Message : {}", messageTypeStr(messageTypes), pCallbackData->messageIdNumber,
        pCallbackData->pMessageIdName == nullptr ? NULL_MSG_ID : UTF8_TO_TCHAR(pCallbackData->pMessageIdName),
        pCallbackData->pMessage == nullptr ? NULL_MSG : UTF8_TO_TCHAR(pCallbackData->pMessage)
    );

    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        LOG_DEBUG("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            LOG_DEBUG(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pQueueLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pQueueLabels[i].pLabelName)
            );
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        LOG_DEBUG("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            LOG_DEBUG(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pCmdBufLabels[i].pLabelName)
            );
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        LOG_DEBUG("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            LOG_DEBUG(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? NULL_OBJ_NAME : UTF8_TO_TCHAR(pCallbackData->pObjects[i].pObjectName)
            );
        }
    }

    return VK_FALSE;
}

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackInfo(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * /*pUserData*/
)
{
    LOG("VulkanDebugUtils", "{}[ID : {}][Name : {}] Message : {}", messageTypeStr(messageTypes), pCallbackData->messageIdNumber,
        pCallbackData->pMessageIdName == nullptr ? NULL_MSG_ID : UTF8_TO_TCHAR(pCallbackData->pMessageIdName),
        pCallbackData->pMessage == nullptr ? NULL_MSG : UTF8_TO_TCHAR(pCallbackData->pMessage));

    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        LOG("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            LOG("VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pQueueLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pQueueLabels[i].pLabelName));
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        LOG("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            LOG("VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pCmdBufLabels[i].pLabelName));
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        LOG("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            LOG("VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? NULL_OBJ_NAME : UTF8_TO_TCHAR(pCallbackData->pObjects[i].pObjectName));
        }
    }

    return VK_FALSE;
}

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackWarn(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * /*pUserData*/
)
{
    LOG_WARN(
        "VulkanDebugUtils", "{}[ID : {}][Name : {}] Message : {}", messageTypeStr(messageTypes), pCallbackData->messageIdNumber,
        pCallbackData->pMessageIdName == nullptr ? NULL_MSG_ID : UTF8_TO_TCHAR(pCallbackData->pMessageIdName),
        pCallbackData->pMessage == nullptr ? NULL_MSG : UTF8_TO_TCHAR(pCallbackData->pMessage)
    );

    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        LOG_WARN("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            LOG_WARN(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pQueueLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pQueueLabels[i].pLabelName)
            );
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        LOG_WARN("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            LOG_WARN(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pCmdBufLabels[i].pLabelName)
            );
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        LOG_WARN("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            LOG_WARN(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? NULL_OBJ_NAME : UTF8_TO_TCHAR(pCallbackData->pObjects[i].pObjectName)
            );
        }
    }

    return VK_FALSE;
}

VkBool32 VulkanDebugLogger::vkDebugUtilsMessengerCallbackError(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * /*pUserData*/
)
{
    LOG_ERROR(
        "VulkanDebugUtils", "{}[ID : {}][Name : {}] Message : {}", messageTypeStr(messageTypes), pCallbackData->messageIdNumber,
        pCallbackData->pMessageIdName == nullptr ? NULL_MSG_ID : UTF8_TO_TCHAR(pCallbackData->pMessageIdName),
        pCallbackData->pMessage == nullptr ? NULL_MSG : UTF8_TO_TCHAR(pCallbackData->pMessage)
    );

    if (pCallbackData->queueLabelCount > 0 && pCallbackData->pQueueLabels[0].pLabelName != nullptr)
    {
        LOG_ERROR("VulkanDebugUtils", "Queues -->");
        for (uint32 i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            LOG_ERROR(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pQueueLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pQueueLabels[i].pLabelName)
            );
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0 && pCallbackData->pCmdBufLabels[0].pLabelName != nullptr)
    {
        LOG_ERROR("VulkanDebugUtils", "Command Buffers -->");
        for (uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            LOG_ERROR(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pCmdBufLabels[i].pLabelName == nullptr ? NULL_LABEL : UTF8_TO_TCHAR(pCallbackData->pCmdBufLabels[i].pLabelName)
            );
        }
    }

    if (pCallbackData->objectCount > 0 && pCallbackData->pObjects[0].pObjectName != nullptr)
    {
        LOG_ERROR("VulkanDebugUtils", "Objects -->");
        for (uint32 i = 0; i < pCallbackData->objectCount; i++)
        {
            LOG_ERROR(
                "VulkanDebugUtils", "        {} : {}", i,
                pCallbackData->pObjects[i].pObjectName == nullptr ? NULL_OBJ_NAME : UTF8_TO_TCHAR(pCallbackData->pObjects[i].pObjectName)
            );
        }
    }

    debugAssert(!"Vulkan Error!");
    return VK_FALSE;
}

bool VulkanDebugLogger::registerDebugLogger(const VkInstance vulkanInstance)
{
#if DEV_BUILD & _VERBOSE
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
#if DEV_BUILD
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
#if DEV_BUILD
    if (!getData().vulkanInstance)
    {
        return;
    }

    if (getData().debugMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().debugMsgrPtr, nullptr);
        getData().debugMsgrPtr = nullptr;
    }
    if (getData().infoMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().infoMsgrPtr, nullptr);
        getData().infoMsgrPtr = nullptr;
    }
    if (getData().warnMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().warnMsgrPtr, nullptr);
        getData().warnMsgrPtr = nullptr;
    }
    if (getData().errorMsgrPtr)
    {
        Vk::vkDestroyDebugUtilsMessengerEXT(getData().vulkanInstance, getData().errorMsgrPtr, nullptr);
        getData().errorMsgrPtr = nullptr;
    }
    getData().vulkanInstance = nullptr;
#endif
}

VulkanDebugGraphics::VulkanDebugGraphics(VulkanDevice *device)
    : ownerDevice(device)
{}

VulkanDebugGraphics::VulkanDebugGraphics(const VulkanDebugGraphics &other)
    : ownerDevice(other.ownerDevice)
{}

VulkanDebugGraphics::VulkanDebugGraphics(VulkanDebugGraphics &&rValue) noexcept
    : ownerDevice(std::move(rValue.ownerDevice))
{}

void VulkanDebugGraphics::operator= (const VulkanDebugGraphics &other) { ownerDevice = other.ownerDevice; }

void VulkanDebugGraphics::operator= (VulkanDebugGraphics &&rValue) noexcept { ownerDevice = std::move(rValue.ownerDevice); }

void VulkanDebugGraphics::markObject(const IVulkanResources *resource) const
{
    if (!resource || resource->getDispatchableHandle() == 0)
    {
        return;
    }

    DEBUG_UTILS_OBJECT_NAME_INFO(objectNameInfo);
    objectNameInfo.objectHandle = resource->getDispatchableHandle();
    objectNameInfo.objectType = resource->getObjectType();
    std::string name{ TCHAR_TO_UTF8(resource->getObjectName().getChar()) };
    objectNameInfo.pObjectName = name.c_str();

    ownerDevice->vkSetDebugUtilsObjectNameEXT(VulkanGraphicsHelper::getDevice(ownerDevice), &objectNameInfo);
}

void VulkanDebugGraphics::markObject(uint64 objectHandle, const String &objectName, VkObjectType objectType) const
{
    if (objectHandle == 0)
    {
        return;
    }

    DEBUG_UTILS_OBJECT_NAME_INFO(objectNameInfo);
    objectNameInfo.objectHandle = objectHandle;
    objectNameInfo.objectType = objectType;
    std::string name{ TCHAR_TO_UTF8(objectName.getChar()) };
    objectNameInfo.pObjectName = name.c_str();

    ownerDevice->vkSetDebugUtilsObjectNameEXT(VulkanGraphicsHelper::getDevice(ownerDevice), &objectNameInfo);
}

void VulkanDebugGraphics::
    beginCmdBufferMarker(VkCommandBuffer commandBuffer, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    std::string cmdBufferName{ TCHAR_TO_UTF8(name.getChar()) };
    label.pLabelName = cmdBufferName.c_str();

    ownerDevice->vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
}

void VulkanDebugGraphics::
    insertCmdBufferMarker(VkCommandBuffer commandBuffer, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    std::string cmdBufferName{ TCHAR_TO_UTF8(name.getChar()) };
    label.pLabelName = cmdBufferName.c_str();

    ownerDevice->vkCmdInsertDebugUtilsLabelEXT(commandBuffer, &label);
}

void VulkanDebugGraphics::endCmdBufferMarker(VkCommandBuffer commandBuffer) const { ownerDevice->vkCmdEndDebugUtilsLabelEXT(commandBuffer); }

void VulkanDebugGraphics::beginQueueMarker(VkQueue queue, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    std::string queueName{ TCHAR_TO_UTF8(name.getChar()) };
    label.pLabelName = queueName.c_str();

    ownerDevice->vkQueueBeginDebugUtilsLabelEXT(queue, &label);
}

void VulkanDebugGraphics::insertQueueMarker(VkQueue queue, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/) const
{
    DEBUG_UTILS_LABEL(label);
    memcpy(&label.color, &color.getColorValue(), sizeof(glm::vec4));
    std::string queueName{ TCHAR_TO_UTF8(name.getChar()) };
    label.pLabelName = queueName.c_str();

    ownerDevice->vkQueueInsertDebugUtilsLabelEXT(queue, &label);
}

void VulkanDebugGraphics::endQueueMarker(VkQueue queue) const { ownerDevice->vkQueueEndDebugUtilsLabelEXT(queue); }

ScopedVulkanCommandMarker::
    ScopedVulkanCommandMarker(VkCommandBuffer commandBuffer, const String &name, const LinearColor &color /* = LinearColorConst::WHITE*/)
{
    cmdBuffer = commandBuffer;
    const VulkanDebugGraphics *graphicsDebugger = VulkanGraphicsHelper::debugGraphics(IVulkanRHIModule::get()->getGraphicsInstance());
    graphicsDebugger->beginCmdBufferMarker(cmdBuffer, name, color);
}

ScopedVulkanCommandMarker::~ScopedVulkanCommandMarker()
{
    const VulkanDebugGraphics *graphicsDebugger = VulkanGraphicsHelper::debugGraphics(IVulkanRHIModule::get()->getGraphicsInstance());
    graphicsDebugger->endCmdBufferMarker(cmdBuffer);
}

ScopedVulkanQueueMarker::ScopedVulkanQueueMarker(VkQueue q, const String &name, const LinearColor &color /* = LinearColorConst::WHITE*/)
{
    queue = q;
    const VulkanDebugGraphics *graphicsDebugger = VulkanGraphicsHelper::debugGraphics(IVulkanRHIModule::get()->getGraphicsInstance());
    graphicsDebugger->beginQueueMarker(queue, name, color);
}

ScopedVulkanQueueMarker::~ScopedVulkanQueueMarker()
{
    const VulkanDebugGraphics *graphicsDebugger = VulkanGraphicsHelper::debugGraphics(IVulkanRHIModule::get()->getGraphicsInstance());
    graphicsDebugger->endQueueMarker(queue);
}
