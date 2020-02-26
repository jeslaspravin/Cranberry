#pragma once
#include "../../../RenderInterface/Resources/QueueResource.h"
#include "../../../Core/String/String.h"
#include "../VulkanMacros.h"
#include "../../Resources/IVulkanResources.h"
#include "../../../Core/Logger/Logger.h"

#include <bitset>
#include "../../../Core/Engine/GameEngine.h"
#include "../../VulkanGraphicsHelper.h"

struct QueueBasePointer
{
    QueueBasePointer() : countPerPriority(1),minAvailablePriority(EQueuePriority::Low)
    {
        for (int32 index = 0; index < EQueuePriority::MaxPriorityEnum; ++index)
        {
            queueBasePointer[index] = nullptr;
            lastQueueIndex[index] = 0;
        }
    }

    VkQueue* queueBasePointer[EQueuePriority::MaxPriorityEnum];
    uint32 lastQueueIndex[EQueuePriority::MaxPriorityEnum];
    uint32 countPerPriority;
    EQueuePriority::Enum minAvailablePriority;
};

template<EQueueFunction QueueType>
class VulkanQueueResource : public QueueResourceBase,public IVulkanResources {
        
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanQueueResource, <QueueType>, QueueResourceBase,)
private:

    int32 queueFamilyPropIndex;
    VkQueueFamilyProperties familyProperty;
    std::vector<float> priorities;
    std::vector<VkQueue> queues;
    QueueBasePointer queuePointer;

private:

    constexpr static VkQueueFlags getSupportedQueueFlag() 
    {
        switch (QueueType)
        {
        case EQueueFunction::Compute:
            return VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT;
            break;
        case EQueueFunction::Graphics:
            return VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
            break;
        case EQueueFunction::Transfer:
            return VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT;
            break;
        case EQueueFunction::Generic:
        case EQueueFunction::Present:
        default:
            return VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT | VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT
                | VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT | VkQueueFlagBits::VK_QUEUE_SPARSE_BINDING_BIT
                | VkQueueFlagBits::VK_QUEUE_PROTECTED_BIT;
            break;
        }
    }
    
public:
    VulkanQueueResource()
        : BaseType()
        , queueFamilyPropIndex(-1)
        , familyProperty()
        , priorities()
        , queues()
        , queuePointer()
    {}

    VulkanQueueResource(const std::vector<VkQueueFamilyProperties>& properties)
        : BaseType()
        , queueFamilyPropIndex(-1)
        , familyProperty()
        , priorities()
        , queues()
        , queuePointer()
    {
        uint32 minBits = VkQueueFlagBits::VK_QUEUE_FLAG_BITS_MAX_ENUM;
        for (int32 i = 0; i < properties.size() ; ++i)
        {
            const VkQueueFamilyProperties& famProp=properties[i];
            uint32 bitFlagsToCheck = famProp.queueFlags & getSupportedQueueFlag();
            if(famProp.queueCount > 0 && bitFlagsToCheck != 0)
            {
                if (bitFlagsToCheck == famProp.queueFlags)
                {
                    queueFamilyPropIndex = i;
                    break;
                }
                else
                {
                    std::bitset<sizeof(int32)> countBit(famProp.queueFlags ^ bitFlagsToCheck);
                    if (minBits > countBit.count())
                    {
                        minBits = (uint32)countBit.count();
                        queueFamilyPropIndex = i;
                    }
                }
            }
        }
        if (queueFamilyPropIndex != -1)
        {
            Logger::debug("VulkanQueueResource", "%s() : Selected queue family at index %d for %s processing",
                __func__, queueFamilyPropIndex, getSupportedQueueName().getChar());
            familyProperty = properties[queueFamilyPropIndex];

            int32 maxQCountPerPriority = familyProperty.queueCount / (uint32)EQueuePriority::MaxPriorityEnum;
            maxQCountPerPriority = maxQCountPerPriority > 1 ? maxQCountPerPriority - 1 : maxQCountPerPriority;// Just to leave some queues not to overload device
            
            int32 totalQueueCount = maxQCountPerPriority * (int32)EQueuePriority::MaxPriorityEnum;

            if (totalQueueCount == 0) // if Queue Count less than EQueuePriority::MaxPriorityEnum
            {
                totalQueueCount = familyProperty.queueCount;
                maxQCountPerPriority = 1;
                queuePointer.minAvailablePriority = (EQueuePriority::Enum)
                    (EQueuePriority::MaxPriorityEnum - totalQueueCount);
            }
            priorities.resize(totalQueueCount);
            
            constexpr float priorityStep = 1 / (float)EQueuePriority::MaxPriorityEnum;
            float priority = 1;
            for (int32 index = totalQueueCount; index > 0; index -= maxQCountPerPriority
                ,priority -= priorityStep)
            {
                auto priorityEnd = priorities.begin() + index;
                auto priorityBegin = priorityEnd - maxQCountPerPriority;
                while (priorityBegin != priorityEnd)
                {
                    (*priorityBegin) = priority;
                    ++priorityBegin;
                }

                //Logger::debug("VulkanQueueResource", "%s() : Filling priority %0.2f from %d to %d of queue priorities",
                //    __func__, priority, index - maxQCountPerPriority, index);
            }

            queuePointer.countPerPriority = maxQCountPerPriority;
            Logger::debug("VulkanQueueResource", "%s() : Using %d queues per priority and %d Total queues for %s",
                __func__, maxQCountPerPriority, totalQueueCount, getSupportedQueueName().getChar());
        }
    }

    VulkanQueueResource(const std::map<uint32,VkQueueFamilyProperties*>& properties)
        : BaseType()
        , queueFamilyPropIndex(-1)
        , familyProperty()
        , priorities()
        , queues()
        , queuePointer()
    {
        uint32 minBits = VkQueueFlagBits::VK_QUEUE_FLAG_BITS_MAX_ENUM;
        for (const std::pair<const uint32,VkQueueFamilyProperties*>& prop:properties)
        {
            const VkQueueFamilyProperties& famProp = *prop.second;
            uint32 bitFlagsToCheck = famProp.queueFlags & getSupportedQueueFlag();
            if (famProp.queueCount > 0 && bitFlagsToCheck != 0)
            {
                if (bitFlagsToCheck == famProp.queueFlags)
                {
                    queueFamilyPropIndex = prop.first;
                    break;
                }
                else
                {
                    std::bitset<sizeof(int32)> countBit(famProp.queueFlags ^ bitFlagsToCheck);
                    if (minBits > countBit.count())
                    {
                        minBits = (uint32)countBit.count();
                        queueFamilyPropIndex = prop.first;
                    }
                }
            }
        }
        if (queueFamilyPropIndex != -1)
        {
            Logger::debug("VulkanQueueResource", "%s() : Selected queue family at index %d for %s processing",
                __func__, queueFamilyPropIndex, getSupportedQueueName().getChar());
            familyProperty = *(properties.find(queueFamilyPropIndex)->second);

            int32 maxQCountPerPriority = familyProperty.queueCount / (uint32)EQueuePriority::MaxPriorityEnum;
            maxQCountPerPriority = maxQCountPerPriority > 1 ? maxQCountPerPriority - 1 : maxQCountPerPriority;// Just to leave some queues not to overload device

            int32 totalQueueCount = maxQCountPerPriority * (int32)EQueuePriority::MaxPriorityEnum;

            if (totalQueueCount == 0) // if Queue Count less than EQueuePriority::MaxPriorityEnum
            {
                totalQueueCount = familyProperty.queueCount;
                maxQCountPerPriority = 1;
                queuePointer.minAvailablePriority = (EQueuePriority::Enum)
                    (EQueuePriority::MaxPriorityEnum - totalQueueCount);
            }
            priorities.resize(totalQueueCount);

            constexpr float priorityStep = 1 / (float)EQueuePriority::MaxPriorityEnum;
            float priority = 1;
            for (int32 index = totalQueueCount; index > 0; index -= maxQCountPerPriority
                , priority -= priorityStep)
            {
                auto priorityEnd = priorities.begin() + index;
                auto priorityBegin = priorityEnd - maxQCountPerPriority;
                while (priorityBegin != priorityEnd)
                {
                    (*priorityBegin) = priority;
                    ++priorityBegin;
                }
            }

            queuePointer.countPerPriority = maxQCountPerPriority;
            Logger::debug("VulkanQueueResource", "%s() : Using %d queues per priority and %d Total queues for %s",
                __func__, maxQCountPerPriority, totalQueueCount, getSupportedQueueName().getChar());
        }
    }

    constexpr static String getSupportedQueueName()
    {
        switch (QueueType)
        {
        case EQueueFunction::Compute:
            return "Compute";
            break;
        case EQueueFunction::Graphics:
            return "Graphics";
                break;
        case EQueueFunction::Transfer:
            return "Transfer";
            break;
        case EQueueFunction::Present:
            return "Present";
            break;
        case EQueueFunction::Generic:
        default:
            return "Generic";
            break;
        }
    }

    bool isValidQueue() const override
    {
        return queueFamilyPropIndex != -1;
    }

    void getQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo) const
    {
        queueCreateInfo.queueFamilyIndex = queueFamilyPropIndex;
        queueCreateInfo.queueCount = (uint32_t)priorities.size();
        queueCreateInfo.pQueuePriorities = priorities.data();
    }

    void cacheQueues(VkDevice logicalDevice,PFN_vkGetDeviceQueue funcPtr)
    {
        queues.resize(priorities.size());
        for (int32 index = 0; index < queues.size(); ++index)
        {
            funcPtr(logicalDevice, queueFamilyPropIndex, index, &queues[index]);
            if (queues[index] == nullptr)
            {
                Logger::error("VulkanQueueResource", "%s() : [%s] Get queue failed for queue family %d at queue index %d"
                    , __func__, getSupportedQueueName().getChar(), queueFamilyPropIndex, index);
            }
            VulkanGraphicsHelper::debugGraphics(gEngine->getRenderApi()->getGraphicsInstance())->markObject(
                (uint64)queues[index], getObjectName().append("Queue_").append(std::to_string(priorities[index])),
                getObjectType());
        }

        int32 qIdx = 0;
        for(uint8 priority = queuePointer.minAvailablePriority;priority < EQueuePriority::MaxPriorityEnum;
            ++priority,qIdx += queuePointer.countPerPriority)
        {
            queuePointer.queueBasePointer[priority] = &queues[qIdx];
        }
    }

    uint32 queueFamilyIndex() { return queueFamilyPropIndex; }

    template <EQueuePriority::Enum Priority>
    constexpr VkQueue getQueueOfPriority()
    {
        uint32 currentQueueIndex = queuePointer.lastQueueIndex[Priority]++;
        queuePointer.lastQueueIndex[Priority] %= queuePointer.countPerPriority;

        return queuePointer.queueBasePointer[Priority][currentQueueIndex];
    }

    String getResourceName() const override
    {
        return getSupportedQueueName();
    }


    String getObjectName() const override
    {
        return getResourceName();
    }

};


namespace VulkanQueueResourceInvoker
{
    template<typename ReturnType, template<typename QueueType> typename Functor,typename ... Args>
    ReturnType invoke(QueueResourceBase* queueRes,Args ...args)
    {

        if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Compute>>())
        {
            return Functor<VulkanQueueResource<EQueueFunction::Compute>*>{}
                (static_cast<VulkanQueueResource<EQueueFunction::Compute>*>(queueRes),args...);
        }
        else if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Graphics>>())
        {
            return Functor<VulkanQueueResource<EQueueFunction::Graphics>*>{}
                (static_cast<VulkanQueueResource<EQueueFunction::Graphics>*>(queueRes), args...);
        }
        else if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Transfer>>())
        {
            return Functor<VulkanQueueResource<EQueueFunction::Transfer>*>{}
                (static_cast<VulkanQueueResource<EQueueFunction::Transfer>*>(queueRes), args...);
        }
        else if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Present>>())
        {
            return Functor<VulkanQueueResource<EQueueFunction::Present>*>{}
            (static_cast<VulkanQueueResource<EQueueFunction::Present>*>(queueRes), args...);
        }
        else if (queueRes->getType()->isChildOf<VulkanQueueResource<EQueueFunction::Generic>>())
        {
            return Functor<VulkanQueueResource<EQueueFunction::Generic>*>{}
                (static_cast<VulkanQueueResource<EQueueFunction::Generic>*>(queueRes), args...);
        }
        Logger::error("VulkanQueueResourceInvoker", "%s() : Invoker failed to find a type", __func__);

        return Functor<VulkanQueueResource<EQueueFunction::Generic>*>{}
            (static_cast<VulkanQueueResource<EQueueFunction::Generic>*>(queueRes), args...);
    }
}


DEFINE_TEMPLATED_VK_GRAPHICS_RESOURCE(VulkanQueueResource, <EQueueFunction QueueType>, <QueueType>,VK_OBJECT_TYPE_QUEUE)

