#pragma once
#include "../../../RenderInterface/Resources/QueueResource.h"
#include "../../../Core/String/String.h"
#include <bitset>



template<EQueueFunction QueueType>
class VulkanQueueResource : public QueueResourceBase {
		
	DECLARE_GRAPHICS_RESOURCE(VulkanQueueResource, <QueueType>, QueueResourceBase,)
private:

	int32 queueFamilyPropIndex;
	VkQueueFamilyProperties familyProperty;
	std::vector<float> priorities;

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
		default:
			return VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT | VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT
				| VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT | VkQueueFlagBits::VK_QUEUE_SPARSE_BINDING_BIT
				| VkQueueFlagBits::VK_QUEUE_PROTECTED_BIT;
			break;
		}
	}
	
public:
	VulkanQueueResource():queueFamilyPropIndex(-1){}

	VulkanQueueResource(const std::vector<VkQueueFamilyProperties>& properties):queueFamilyPropIndex(-1)
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
				//	__func__, priority, index - maxQCountPerPriority, index);
			}

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
		case EQueueFunction::Generic:
		default:
			return "Generic";
			break;
		}
	}

	bool isValidQueue() const
	{
		return queueFamilyPropIndex != -1;
	}

	void getQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo) const
	{
		queueCreateInfo.queueFamilyIndex = queueFamilyPropIndex;
		queueCreateInfo.queueCount = (uint32_t)priorities.size();
		queueCreateInfo.pQueuePriorities = priorities.data();
	}
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(VulkanQueueResource, <EQueueFunction QueueType>, <QueueType>)