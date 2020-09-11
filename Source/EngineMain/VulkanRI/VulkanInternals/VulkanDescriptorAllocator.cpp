#include "VulkanDescriptorAllocator.h"
#include "VulkanMacros.h"
#include "VulkanDevice.h"
#include "../VulkanGraphicsHelper.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Logger/Logger.h"
#include "../../Core/Math/Math.h"

#include <array>


bool DescriptorPoolSizeLessThan::operator()(const VkDescriptorPoolSize& lhs, const VkDescriptorPoolSize& rhs) const
{
    return lhs.type == rhs.type ? lhs.descriptorCount < rhs.descriptorCount : lhs.type < rhs.type;
}

bool DescriptorsSetQueryLessThan::recursivelyCompare(std::set<VkDescriptorPoolSize>::const_iterator& lhsItr
    , std::set<VkDescriptorPoolSize>::const_iterator& lhsEnd, std::set<VkDescriptorPoolSize>::const_iterator& rhsItr
    , std::set<VkDescriptorPoolSize>::const_iterator& rhsEnd) const
{
    uint32 lhsEnded = lhsItr == lhsEnd ? 1 : 0;
    uint32 rhsEnded = rhsItr == rhsEnd ? 1 : 0;

    if (lhsEnded == 0 && rhsEnded == 0)
    {
        return lhsItr->type == rhsItr->type ? recursivelyCompare(++lhsItr, lhsEnd, ++rhsItr, rhsEnd) : lhsItr->type < rhsItr->type;
    }

    return rhsEnded == lhsEnded ? true : lhsEnded > rhsEnded;
}

bool DescriptorsSetQueryLessThan::operator()(const DescriptorsSetQuery& lhs, const DescriptorsSetQuery& rhs) const
{
    auto lhsItr = lhs.supportedTypes.cbegin();
    auto lhsEnd = lhs.supportedTypes.cend();
    auto rhsItr = rhs.supportedTypes.cbegin();
    auto rhsEnd = rhs.supportedTypes.cend();
    return recursivelyCompare(lhsItr, lhsEnd, rhsItr, rhsEnd);
}


bool VulkanDescriptorsSetAllocator::isSupportedPool(std::vector<VkDescriptorSet>& availableSets, const VulkanDescriptorsSetAllocatorInfo& allocationPool, const DescriptorsSetQuery& query
    , const uint32& setsCount) const
{
    bool bSizeQualification = true;
    for (const VkDescriptorPoolSize& poolSize : query.supportedTypes)
    {
        bSizeQualification = bSizeQualification && poolSize.descriptorCount <= allocationPool.typeCountMap.at(poolSize.type);
    }

    if (bSizeQualification)
    {
        if (allocationPool.allocatedSets.size() + setsCount <= allocationPool.maxSets)// if there is still possibility to allocate any set then do that rather than searching
        {
            availableSets.clear();
            return true;
        }
        else
        {
            availableSets.reserve(setsCount);
            for (const VkDescriptorSet& descriptorsSet : allocationPool.availableSets)
            {
                const auto& descriptorsPoolSizes = allocationPool.allocatedSets.at(descriptorsSet).supportedTypes;
                if (descriptorsPoolSizes.size() != query.supportedTypes.size())
                {
                    continue;
                }

                bool bIsSuitableDescsSet = true;
                // Check if set 100% intersection
                for (auto descPoolSizeItr = descriptorsPoolSizes.cbegin()
                    , queryPoolSizeItr = query.supportedTypes.cbegin(); descPoolSizeItr != descriptorsPoolSizes.cend()
                    ; ++descPoolSizeItr, ++queryPoolSizeItr)
                {
                    if (descPoolSizeItr->descriptorCount != queryPoolSizeItr->descriptorCount
                        || descPoolSizeItr->type != queryPoolSizeItr->type)
                    {
                        bIsSuitableDescsSet = false;
                        break;
                    }
                }

                if (bIsSuitableDescsSet)
                {
                    availableSets.push_back(descriptorsSet);
                    if (availableSets.size() >= setsCount)
                    {
                        availableSets.shrink_to_fit();
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

VkDescriptorSet VulkanDescriptorsSetAllocator::allocateSetFromPool(VulkanDescriptorsSetAllocatorInfo& allocationPool
    , const VkDescriptorSetLayout& descriptorsSetLayout) const
{
    DESCRIPTOR_SET_ALLOCATE_INFO(descsSetAllocInfo);
    descsSetAllocInfo.descriptorPool = allocationPool.pool;
    descsSetAllocInfo.descriptorSetCount = 1;
    descsSetAllocInfo.pSetLayouts = &descriptorsSetLayout;

    VkDescriptorSet descsSet;
    fatalAssert(ownerDevice->vkAllocateDescriptorSets(VulkanGraphicsHelper::getDevice(ownerDevice), &descsSetAllocInfo, &descsSet) == VK_SUCCESS, "Allocating new descriptors set failed");
    return descsSet;
}

bool VulkanDescriptorsSetAllocator::allocateSetsFromPool(std::vector<VkDescriptorSet>& allocatedSets
    , VulkanDescriptorsSetAllocatorInfo& allocationPool, const std::vector<VkDescriptorSetLayout>& layouts) const
{
    allocatedSets.resize(layouts.size());

    DESCRIPTOR_SET_ALLOCATE_INFO(descsSetAllocInfo);
    descsSetAllocInfo.descriptorPool = allocationPool.pool;
    descsSetAllocInfo.descriptorSetCount = uint32(layouts.size());
    descsSetAllocInfo.pSetLayouts = layouts.data();

    return ownerDevice->vkAllocateDescriptorSets(VulkanGraphicsHelper::getDevice(ownerDevice), &descsSetAllocInfo, allocatedSets.data()) == VK_SUCCESS;
}

VulkanDescriptorsSetAllocatorInfo& VulkanDescriptorsSetAllocator::createNewPool(const DescriptorsSetQuery& query
    , const uint32& setsCount, std::vector<VulkanDescriptorsSetAllocatorInfo>& poolGroup) const
{
    uint32 poolGroupIndex = uint32(poolGroup.size());
    poolGroup.resize(poolGroupIndex + 1);
    VulkanDescriptorsSetAllocatorInfo& allocationPool = poolGroup[poolGroupIndex];
    allocationPool.idlingDuration = 0;

    std::vector<VkDescriptorPoolSize> descriptorsSetPoolSizes(query.supportedTypes.size());

    DESCRIPTOR_POOL_CREATE_INFO(descsSetPoolCreateInfo);
    descsSetPoolCreateInfo.maxSets = allocationPool.maxSets = Math::max(DESCRIPTORS_SET_POOL_MAX_SETS,setsCount);
    descsSetPoolCreateInfo.poolSizeCount = uint32(descriptorsSetPoolSizes.size());
    descsSetPoolCreateInfo.pPoolSizes = descriptorsSetPoolSizes.data();

    auto queryDescPoolSizeItr = query.supportedTypes.cbegin();
    for (uint32 i = 0; i <= descsSetPoolCreateInfo.poolSizeCount; ++i, ++queryDescPoolSizeItr)
    {
        allocationPool.typeCountMap[queryDescPoolSizeItr->type] = queryDescPoolSizeItr->descriptorCount;
        descriptorsSetPoolSizes[i] = *queryDescPoolSizeItr;
    }

    fatalAssert(ownerDevice->vkCreateDescriptorPool(VulkanGraphicsHelper::getDevice(ownerDevice), &descsSetPoolCreateInfo
        , nullptr, &allocationPool.pool) == VK_SUCCESS, "pool creation failed");

    return allocationPool;
}

VulkanDescriptorsSetAllocatorInfo& VulkanDescriptorsSetAllocator::findOrCreateAllocPool(std::vector<VkDescriptorSet>& availableSets
    , const DescriptorsSetQuery& query, const uint32& setsCount)
{
    availableSets.clear();
    size_t setsRequiredCount = setsCount;

    VulkanDescriptorsSetAllocatorInfo* allocationPool = nullptr;
    auto allocationPoolsItr = availablePools.find(query);
    if (allocationPoolsItr != availablePools.end())
    {
        for (VulkanDescriptorsSetAllocatorInfo& availableAllocationInfo : allocationPoolsItr->second)
        {
            std::vector<VkDescriptorSet> tempSets;
            if (isSupportedPool(tempSets, availableAllocationInfo, query, uint32(setsRequiredCount)))
            {
                Logger::debug("DescriptorsSetAllocator", "%s() : Found existing pool that supports query, obtained %d Descriptors set",__func__, uint32(tempSets.size()));
                allocationPool = &availableAllocationInfo;
                // If pool has enough capacity to allocate then support will be true and sets returned will be 0
                if (tempSets.empty())
                {
                    break;
                }
            }
            if (!tempSets.empty())
            {
                availableSets.insert(availableSets.end(), tempSets.cbegin(), tempSets.cend());
                setsRequiredCount -= tempSets.size();
                // Clearing the sets from available descriptors sets set
                for (VkDescriptorSet& availableSet : tempSets)
                {
                    availableAllocationInfo.availableSets.erase(availableSet);
                }
            }
            if (setsRequiredCount <= 0)
            {
                allocationPool = &availableAllocationInfo;
                setsRequiredCount = 0;
                break;
            }
        }
    }
    if (allocationPool == nullptr && setsRequiredCount != 0)
    {
        Logger::debug("DescriptorsSetAllocator", "Creating new pool that supports query");
        allocationPool = &createNewPool(query, uint32(setsRequiredCount), availablePools[query]);
    }
    debugAssert(allocationPool != nullptr);
    allocationPool->idlingDuration = 0;
    return *allocationPool;
}

VulkanDescriptorsSetAllocatorInfo& VulkanDescriptorsSetAllocator::findOrCreateAllocPool(const DescriptorsSetQuery& query, const uint32& setsCount)
{
    VulkanDescriptorsSetAllocatorInfo* allocationPool = nullptr;
    auto allocationPoolsItr = availablePools.find(query);
    if (allocationPoolsItr != availablePools.end())
    {
        for (VulkanDescriptorsSetAllocatorInfo& availableAllocationInfo : allocationPoolsItr->second)
        {
            std::vector<VkDescriptorSet> tempSets;
            // If pool has enough capacity to allocate then support will be true
            if (isSupportedPool(tempSets, availableAllocationInfo, query, setsCount) && tempSets.empty())
            {
                Logger::debug("DescriptorsSetAllocator", "%s() : Found existing pool that supports query", __func__);
                allocationPool = &availableAllocationInfo;
                break;
            }
        }
    }
    if (allocationPool == nullptr)
    {
        Logger::debug("DescriptorsSetAllocator", "%s() : Creating new pool that supports query", __func__);
        allocationPool = &createNewPool(query, setsCount, availablePools[query]);
    }
    debugAssert(allocationPool != nullptr);
    allocationPool->idlingDuration = 0;
    return *allocationPool;
}

void VulkanDescriptorsSetAllocator::resetAllocationPool(VulkanDescriptorsSetAllocatorInfo& allocationPool) const
{
    ownerDevice->vkResetDescriptorPool(VulkanGraphicsHelper::getDevice(ownerDevice), allocationPool.pool, 0);
    allocationPool.allocatedSets.clear();
    allocationPool.availableSets.clear();
    allocationPool.idlingDuration = 0;
}

VulkanDescriptorsSetAllocator::VulkanDescriptorsSetAllocator(VulkanDevice* device)
    : ownerDevice(device)
{
    std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1> globalDescriptorsSetPoolSizes;
    for (uint32 i = VK_DESCRIPTOR_TYPE_SAMPLER; i <= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; ++i)
    {
        globalDescriptorsSetPoolSizes[i].type = VkDescriptorType(i);
        globalDescriptorsSetPoolSizes[i].descriptorCount = DESCRIPTORS_COUNT_PER_SET;
        globalPool.typeCountMap[globalDescriptorsSetPoolSizes[i].type] = globalDescriptorsSetPoolSizes[i].descriptorCount;
    }
    // Since currently we are not supporting array of structures
    globalDescriptorsSetPoolSizes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER].descriptorCount = 1;
    globalDescriptorsSetPoolSizes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER].descriptorCount = 1;

    DESCRIPTOR_POOL_CREATE_INFO(globalPoolCreateInfo);
    globalPoolCreateInfo.poolSizeCount = uint32(globalDescriptorsSetPoolSizes.size());
    globalPoolCreateInfo.pPoolSizes = globalDescriptorsSetPoolSizes.data();
    globalPoolCreateInfo.maxSets = globalPool.maxSets = DESCRIPTORS_SET_POOL_MAX_SETS;

    fatalAssert(ownerDevice->vkCreateDescriptorPool(VulkanGraphicsHelper::getDevice(ownerDevice), &globalPoolCreateInfo
        , nullptr, &globalPool.pool) == VK_SUCCESS, "Global pool creation failed");
}

VulkanDescriptorsSetAllocator::~VulkanDescriptorsSetAllocator()
{
    VkDevice device = VulkanGraphicsHelper::getDevice(ownerDevice);
    ownerDevice->vkDestroyDescriptorPool(device, globalPool.pool, nullptr);
    globalPool.allocatedSets.clear();
    globalPool.availableSets.clear();
    globalPool.pool = nullptr;
    globalPool.typeCountMap.clear();

    for (auto allocationPoolsItr = availablePools.begin(); allocationPoolsItr != availablePools.end(); ++allocationPoolsItr)
    {
        for (VulkanDescriptorsSetAllocatorInfo& allocationPool : allocationPoolsItr->second)
        {
            ownerDevice->vkDestroyDescriptorPool(device, allocationPool.pool, nullptr);
        }
    }
    availablePools.clear();
}

VkDescriptorSet VulkanDescriptorsSetAllocator::allocDescriptorsSet(const DescriptorsSetQuery& query, const VkDescriptorSetLayout& descriptorsSetLayout)
{
    std::vector<VkDescriptorSet> chooseSets;
    if (isSupportedPool(chooseSets, globalPool, query, 1))
    {
        if (chooseSets.empty())
        {
            Logger::debug("DescriptorsSetAllocator", "Allocating set from global descriptors set pool");
            chooseSets.push_back(allocateSetFromPool(globalPool, descriptorsSetLayout));
            globalPool.allocatedSets[chooseSets[0]] = query;
        }
        else
        {
            Logger::debug("DescriptorsSetAllocator", "Fetching from available sets of global descriptors set pool");
            globalPool.availableSets.erase(chooseSets[0]);
        }
    }

    if (chooseSets.empty())
    {
        VulkanDescriptorsSetAllocatorInfo& allocationPool = findOrCreateAllocPool(chooseSets, query, 1);

        if (chooseSets.empty())
        {
            Logger::debug("DescriptorsSetAllocator", "Allocating set from non global pool");
            chooseSets.push_back(allocateSetFromPool(allocationPool, descriptorsSetLayout));
            allocationPool.allocatedSets[chooseSets[0]] = query;
        }
    }
    return chooseSets[0];
}

bool VulkanDescriptorsSetAllocator::allocDescriptorsSets(std::vector<VkDescriptorSet>& sets, const DescriptorsSetQuery& query
    , const std::vector<VkDescriptorSetLayout>& layouts)
{
    sets.clear();
    VulkanDescriptorsSetAllocatorInfo& allocationPool = findOrCreateAllocPool(query, uint32(layouts.size()));

    if (!allocateSetsFromPool(sets, allocationPool, layouts))
    {
        Logger::error("DescriptorsSetAllocator", "%s() : Failed allocating required sets", __func__);
        return false;
    }
    return true;
}

bool VulkanDescriptorsSetAllocator::allocDescriptorsSets(std::vector<VkDescriptorSet>& sets, const DescriptorsSetQuery& query
    , const VkDescriptorSetLayout& layout, const uint32& setsCount)
{
    sets.clear();
    {
        std::vector<VkDescriptorSet> chooseSets;
        VulkanDescriptorsSetAllocatorInfo& allocationPool = findOrCreateAllocPool(chooseSets, query, setsCount);

        if (chooseSets.size() != setsCount)
        {
            uint32 remainingSetsCount = setsCount - uint32(chooseSets.size());
            Logger::debug("DescriptorsSetAllocator", "%s() : Allocating remaining %d required sets", __func__, remainingSetsCount);

            std::vector<VkDescriptorSetLayout> layouts;
            layouts.assign(remainingSetsCount, layout);
            if (!allocateSetsFromPool(sets, allocationPool, layouts))
            {
                Logger::error("DescriptorsSetAllocator", "%s() : Failed allocating required sets", __func__);
                return false;
            }
        }
        sets.insert(sets.end(), chooseSets.cbegin(), chooseSets.cend());
    }
    return true;
}

void VulkanDescriptorsSetAllocator::releaseDescriptorsSet(VkDescriptorSet descriptorSet)
{
    auto descriptorSetItr = globalPool.allocatedSets.find(descriptorSet);

    if (descriptorSetItr == globalPool.allocatedSets.end())
    {
        for (auto allocationPoolsItr = availablePools.begin(); allocationPoolsItr != availablePools.end(); ++allocationPoolsItr)
        {
            for (VulkanDescriptorsSetAllocatorInfo& allocationPool : allocationPoolsItr->second)
            {
                descriptorSetItr = allocationPool.allocatedSets.find(descriptorSet);
                if (descriptorSetItr != allocationPool.allocatedSets.end())
                {
                    allocationPool.availableSets.insert(descriptorSet);
                    return;
                }
            }
        }
    }
    else
    {
        globalPool.availableSets.insert(descriptorSet);
    }
}

void VulkanDescriptorsSetAllocator::tick(const float& deltaTime)
{
    for (auto allocationPoolsItr = availablePools.begin(); allocationPoolsItr != availablePools.end(); ++allocationPoolsItr)
    {
        for (VulkanDescriptorsSetAllocatorInfo& allocationPool : allocationPoolsItr->second)
        {
            if (!allocationPool.allocatedSets.empty() && allocationPool.availableSets.size() == allocationPool.allocatedSets.size())
            {
                allocationPool.idlingDuration += deltaTime;
            }

            if (allocationPool.idlingDuration >= MAX_IDLING_DURATION)
            {
                resetAllocationPool(allocationPool);
            }
        }
    }
}