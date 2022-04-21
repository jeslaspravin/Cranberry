/*!
 * \file VulkanDescriptorAllocator.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/VulkanDescriptorAllocator.h"
#include "Logger/Logger.h"
#include "Math/Math.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanMacros.h"

#include <array>

bool DescriptorPoolSizeLessThan::operator()(const VkDescriptorPoolSize &lhs, const VkDescriptorPoolSize &rhs) const
{
    // Not using type count as the query pool's descriptors type will be unique and merged at calling
    // code return lhs.type == rhs.type ? lhs.descriptorCount < rhs.descriptorCount : lhs.type <
    // rhs.type;
    return lhs.type < rhs.type;
}

bool DescriptorsSetQueryLessThan::recursivelyCompare(
    std::set<VkDescriptorPoolSize>::const_iterator &lhsItr, std::set<VkDescriptorPoolSize>::const_iterator &lhsEnd,
    std::set<VkDescriptorPoolSize>::const_iterator &rhsItr, std::set<VkDescriptorPoolSize>::const_iterator &rhsEnd
) const
{
    uint32 lhsEnded = lhsItr == lhsEnd ? 1 : 0;
    uint32 rhsEnded = rhsItr == rhsEnd ? 1 : 0;

    if ((lhsEnded + rhsEnded) == 0)
    {
        if (lhsItr->type > rhsItr->type)
        {
            return false;
        }
        else if (lhsItr->type == rhsItr->type)
        {
            ++rhsItr;
        }
        // Lhstype is small/Equal to Rhstype increment lhs
        ++lhsItr;
        return recursivelyCompare(lhsItr, lhsEnd, rhsItr, rhsEnd);
    }

    // Rhs never reaches end unless all in rhs is matched with lhs, so lhs is obviously not less that
    // rhs. If Rhs is not end then lhs must have reached end here so it means lhs exhausted all types
    // that are smaller than rhs head so obviously small
    return rhsEnded != 0 ? false : true;
}

bool DescriptorsSetQueryLessThan::operator()(const DescriptorsSetQuery &lhs, const DescriptorsSetQuery &rhs) const
{
    auto lhsItr = lhs.supportedTypes.cbegin();
    auto lhsEnd = lhs.supportedTypes.cend();
    auto rhsItr = rhs.supportedTypes.cbegin();
    auto rhsEnd = rhs.supportedTypes.cend();
    bool bIsLhsSmall = lhs.bHasBindless < rhs.bHasBindless;
    if (lhs.bHasBindless == rhs.bHasBindless)
    {
        // If lhs is small in count then its largest must be smaller than rhs largest to be
        // considered to be smaller than rhs
        bIsLhsSmall
            = (lhs.supportedTypes.size() < rhs.supportedTypes.size() ? ((--lhsEnd)->type < (--rhsEnd)->type)
                                                                     : recursivelyCompare(lhsItr, lhsEnd, rhsItr, rhsEnd));
    }
    return bIsLhsSmall;
}

std::vector<VulkanDescriptorsSetAllocatorInfo *> VulkanDescriptorsSetAllocator::findInAvailablePool(const DescriptorsSetQuery &query)
{
    std::vector<VulkanDescriptorsSetAllocatorInfo *> allocationPools;

    auto allocationPoolsItr = availablePools.find(query);
    if (allocationPoolsItr != availablePools.end())
    {
        std::vector<VkDescriptorPoolSize> outSet;
        outSet.resize(Math::min(query.supportedTypes.size(), allocationPoolsItr->first.supportedTypes.size()));
        auto outEndItr = std::set_intersection(
            query.supportedTypes.cbegin(), query.supportedTypes.cend(), allocationPoolsItr->first.supportedTypes.cbegin(),
            allocationPoolsItr->first.supportedTypes.cend(), outSet.begin(), DescriptorPoolSizeLessThan{}
        );

        if ((outEndItr - outSet.begin()) == query.supportedTypes.size())
        {
            for (VulkanDescriptorsSetAllocatorInfo &allocationPool : allocationPoolsItr->second)
            {
                bool bSatisfiesQuery = true;
                for (const VkDescriptorPoolSize &reqDescriptor : query.supportedTypes)
                {
                    bSatisfiesQuery
                        = bSatisfiesQuery && (allocationPool.typeCountMap.find(reqDescriptor.type) != allocationPool.typeCountMap.cend());
                }

                if (bSatisfiesQuery)
                {
                    allocationPools.emplace_back(&allocationPool);
                }
            }
        }
    }
    return allocationPools;
}

bool VulkanDescriptorsSetAllocator::isSupportedPool(
    std::vector<VkDescriptorSet> &availableSets, const VulkanDescriptorsSetAllocatorInfo &allocationPool, const DescriptorsSetQuery &query,
    const uint32 &setsCount
) const
{
    bool bSizeQualification = true;
    for (const VkDescriptorPoolSize &poolSize : query.supportedTypes)
    {
        bSizeQualification = bSizeQualification && poolSize.descriptorCount <= allocationPool.typeCountMap.at(poolSize.type);
    }

    if (bSizeQualification)
    {
        if (allocationPool.allocatedSets.size() + setsCount <= allocationPool.maxSets) // if there is still possibility to allocate any set then
                                                                                       // do that rather than searching
        {
            availableSets.clear();
            return true;
        }
        else
        {
            availableSets.reserve(setsCount);
            for (const VkDescriptorSet &descriptorsSet : allocationPool.availableSets)
            {
                const DescriptorsSetQuery &allocatedSetQuery = allocationPool.allocatedSets.at(descriptorsSet);
                // If requested type count is more that allocated size then no useful
                if (allocatedSetQuery.supportedTypes.size() < query.supportedTypes.size())
                {
                    continue;
                }

                bool bIsSuitableDescsSet = true;
                // Check if set 100% intersection

                // Below check using descriptor's total pool size won't provide proper value
                // when binding level count varies or if stage used varies
                // for (auto descPoolSizeItr = descriptorsPoolSizes.cbegin()
                //    , queryPoolSizeItr = query.supportedTypes.cbegin(); descPoolSizeItr !=
                //    descriptorsPoolSizes.cend() ; ++descPoolSizeItr, ++queryPoolSizeItr)
                //{
                //    if (descPoolSizeItr->descriptorCount !=
                //    queryPoolSizeItr->descriptorCount
                //        || descPoolSizeItr->type != queryPoolSizeItr->type)
                //    {
                //        bIsSuitableDescsSet = false;
                //        break;
                //    }
                //}
                if (query.allocatedBindings != allocatedSetQuery.allocatedBindings)
                {
                    auto queryBindingItr = query.allocatedBindings->cbegin();
                    auto allocBindingItr = allocatedSetQuery.allocatedBindings->cbegin();
                    while (queryBindingItr != query.allocatedBindings->cend() && allocBindingItr != allocatedSetQuery.allocatedBindings->cend())
                    {
                        // If allocated binding is already larger than query binding
                        // then allocated do not support that binding index
                        if (queryBindingItr->binding < allocBindingItr->binding)
                        {
                            bIsSuitableDescsSet = false;
                            break;
                        }
                        // If allocated binding has binding at lower indices skip
                        // them
                        else if (queryBindingItr->binding > allocBindingItr->binding)
                        {
                            ++allocBindingItr;
                        }
                        // Same binding case, if Descriptors do not match then fail
                        // this set
                        else if (queryBindingItr->descriptorType != allocBindingItr->descriptorType
                                 || queryBindingItr->descriptorCount > allocBindingItr->descriptorCount // If required
                                                                                                        // descriptors count
                                                                                                        // is greater than
                                                                                                        // allocated set's
                                 || BIT_NOT_SET(allocBindingItr->stageFlags,
                                     queryBindingItr->stageFlags)) // If all queried shader
                                                                   // stages are not supported
                        {
                            bIsSuitableDescsSet = false;
                            break;
                        }
                        else
                        {
                            ++allocBindingItr;
                            ++queryBindingItr;
                        }
                    }
                    // Query iterator must have ended for it to be suitable
                    bIsSuitableDescsSet = bIsSuitableDescsSet && (queryBindingItr == query.allocatedBindings->cend());
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

VkDescriptorSet VulkanDescriptorsSetAllocator::allocateSetFromPool(
    VulkanDescriptorsSetAllocatorInfo &allocationPool, const VkDescriptorSetLayout &descriptorsSetLayout
) const
{
    DESCRIPTOR_SET_ALLOCATE_INFO(descsSetAllocInfo);
    descsSetAllocInfo.descriptorPool = allocationPool.pool;
    descsSetAllocInfo.descriptorSetCount = 1;
    descsSetAllocInfo.pSetLayouts = &descriptorsSetLayout;

    VkDescriptorSet descsSet;
    fatalAssert(
        ownerDevice->vkAllocateDescriptorSets(VulkanGraphicsHelper::getDevice(ownerDevice), &descsSetAllocInfo, &descsSet) == VK_SUCCESS,
        "Allocating new descriptors set failed"
    );
    return descsSet;
}

bool VulkanDescriptorsSetAllocator::allocateSetsFromPool(
    std::vector<VkDescriptorSet> &allocatedSets, VulkanDescriptorsSetAllocatorInfo &allocationPool,
    const std::vector<VkDescriptorSetLayout> &layouts
) const
{
    allocatedSets.resize(layouts.size());

    DESCRIPTOR_SET_ALLOCATE_INFO(descsSetAllocInfo);
    descsSetAllocInfo.descriptorPool = allocationPool.pool;
    descsSetAllocInfo.descriptorSetCount = uint32(layouts.size());
    descsSetAllocInfo.pSetLayouts = layouts.data();

    return ownerDevice->vkAllocateDescriptorSets(VulkanGraphicsHelper::getDevice(ownerDevice), &descsSetAllocInfo, allocatedSets.data())
           == VK_SUCCESS;
}

VulkanDescriptorsSetAllocatorInfo &VulkanDescriptorsSetAllocator::createNewPool(
    const DescriptorsSetQuery &query, const uint32 &setsCount, std::vector<VulkanDescriptorsSetAllocatorInfo> &poolGroup
) const
{
    uint32 poolGroupIndex = uint32(poolGroup.size());
    poolGroup.resize(poolGroupIndex + 1);
    VulkanDescriptorsSetAllocatorInfo &allocationPool = poolGroup[poolGroupIndex];
    allocationPool.idlingDuration = 0;

    std::vector<VkDescriptorPoolSize> descriptorsSetPoolSizes(query.supportedTypes.size());
    DESCRIPTOR_POOL_CREATE_INFO(descsSetPoolCreateInfo);
    descsSetPoolCreateInfo.flags |= query.bHasBindless && (GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_AFTER_BIND)
                                        ? VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT
                                        : 0;
    descsSetPoolCreateInfo.maxSets = allocationPool.maxSets = Math::max(DESCRIPTORS_SET_POOL_MAX_SETS, setsCount);
    descsSetPoolCreateInfo.poolSizeCount = uint32(descriptorsSetPoolSizes.size());
    descsSetPoolCreateInfo.pPoolSizes = descriptorsSetPoolSizes.data();

    auto queryDescPoolSizeItr = query.supportedTypes.cbegin();
    for (uint32 i = 0; i < descsSetPoolCreateInfo.poolSizeCount; ++i, ++queryDescPoolSizeItr)
    {
        allocationPool.typeCountMap[queryDescPoolSizeItr->type] = queryDescPoolSizeItr->descriptorCount;
        descriptorsSetPoolSizes[i] = *queryDescPoolSizeItr;
    }

    fatalAssert(
        ownerDevice->vkCreateDescriptorPool(
            VulkanGraphicsHelper::getDevice(ownerDevice), &descsSetPoolCreateInfo, nullptr, &allocationPool.pool
        ) == VK_SUCCESS,
        "pool creation failed"
    );

    return allocationPool;
}

VulkanDescriptorsSetAllocatorInfo &VulkanDescriptorsSetAllocator::findOrCreateAllocPool(
    std::vector<VkDescriptorSet> &availableSets, const DescriptorsSetQuery &query, const uint32 &setsCount
)
{
    availableSets.clear();
    size_t setsRequiredCount = setsCount;

    VulkanDescriptorsSetAllocatorInfo *allocationPool = nullptr;
    std::vector<VulkanDescriptorsSetAllocatorInfo *> allocationPoolsFound = findInAvailablePool(query);
    if (!allocationPoolsFound.empty())
    {
        for (VulkanDescriptorsSetAllocatorInfo *availableAllocationInfo : allocationPoolsFound)
        {
            std::vector<VkDescriptorSet> tempSets;
            if (isSupportedPool(tempSets, *availableAllocationInfo, query, uint32(setsRequiredCount)))
            {
                LOG_DEBUG(
                    "DescriptorsSetAllocator",
                    "%s() : Found existing pool that supports query, obtained %d existing "
                    "Descriptors set",
                    __func__, uint32(tempSets.size())
                );
                allocationPool = availableAllocationInfo;
                // If pool has enough capacity to allocate then support will be true and sets
                // returned will be 0
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
                for (VkDescriptorSet &availableSet : tempSets)
                {
                    availableAllocationInfo->availableSets.erase(availableSet);
                }
            }
            if (setsRequiredCount <= 0)
            {
                allocationPool = availableAllocationInfo;
                setsRequiredCount = 0;
                break;
            }
        }
    }
    if (allocationPool == nullptr && setsRequiredCount != 0)
    {
        LOG_DEBUG("DescriptorsSetAllocator", "Creating new pool that supports query");
        allocationPool = &createNewPool(query, uint32(setsRequiredCount), availablePools[query]);
    }
    debugAssert(allocationPool != nullptr);
    allocationPool->idlingDuration = 0;
    return *allocationPool;
}

VulkanDescriptorsSetAllocatorInfo &
    VulkanDescriptorsSetAllocator::findOrCreateAllocPool(const DescriptorsSetQuery &query, const uint32 &setsCount)
{
    VulkanDescriptorsSetAllocatorInfo *allocationPool = nullptr;
    std::vector<VulkanDescriptorsSetAllocatorInfo *> allocationPoolsFound = findInAvailablePool(query);
    if (!allocationPoolsFound.empty())
    {
        for (VulkanDescriptorsSetAllocatorInfo *availableAllocationInfo : allocationPoolsFound)
        {
            std::vector<VkDescriptorSet> tempSets;
            // If pool has enough capacity to allocate then support will be true
            if (isSupportedPool(tempSets, *availableAllocationInfo, query, setsCount) && tempSets.empty())
            {
                LOG_DEBUG("DescriptorsSetAllocator", "%s() : Found existing pool that supports query", __func__);
                allocationPool = availableAllocationInfo;
                break;
            }
        }
    }
    if (allocationPool == nullptr)
    {
        LOG_DEBUG("DescriptorsSetAllocator", "%s() : Creating new pool that supports query", __func__);
        allocationPool = &createNewPool(query, setsCount, availablePools[query]);
    }
    debugAssert(allocationPool != nullptr);
    allocationPool->idlingDuration = 0;
    return *allocationPool;
}

void VulkanDescriptorsSetAllocator::resetAllocationPool(VulkanDescriptorsSetAllocatorInfo &allocationPool) const
{
    ownerDevice->vkResetDescriptorPool(VulkanGraphicsHelper::getDevice(ownerDevice), allocationPool.pool, 0);
    allocationPool.allocatedSets.clear();
    allocationPool.availableSets.clear();
    allocationPool.idlingDuration = 0;
}

VulkanDescriptorsSetAllocator::VulkanDescriptorsSetAllocator(VulkanDevice *device)
    : ownerDevice(device)
    , emptyDescriptor(nullptr)
{
    // std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1>
    // globalDescriptorsSetPoolSizes; for (uint32 i = VK_DESCRIPTOR_TYPE_SAMPLER; i <=
    // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; ++i)
    //{
    //     globalDescriptorsSetPoolSizes[i].type = VkDescriptorType(i);
    //     globalDescriptorsSetPoolSizes[i].descriptorCount = DESCRIPTORS_COUNT_PER_SET;
    //     globalPool.typeCountMap[globalDescriptorsSetPoolSizes[i].type] =
    //     globalDescriptorsSetPoolSizes[i].descriptorCount;
    // }
    //// Since currently we are not supporting array of structures
    // globalDescriptorsSetPoolSizes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER].descriptorCount = 1;
    // globalDescriptorsSetPoolSizes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER].descriptorCount = 1;

    // DESCRIPTOR_POOL_CREATE_INFO(globalPoolCreateInfo);
    // globalPoolCreateInfo.poolSizeCount = uint32(globalDescriptorsSetPoolSizes.size());
    // globalPoolCreateInfo.pPoolSizes = globalDescriptorsSetPoolSizes.data();
    // globalPoolCreateInfo.maxSets = globalPool.maxSets = DESCRIPTORS_SET_POOL_MAX_SETS;

    // fatalAssert(ownerDevice->vkCreateDescriptorPool(VulkanGraphicsHelper::getDevice(ownerDevice),
    // &globalPoolCreateInfo
    //     , nullptr, &globalPool.pool) == VK_SUCCESS, "Global pool creation failed");

    // Empty descriptor set creation
    {
        DESCRIPTOR_SET_LAYOUT_CREATE_INFO(emptyLayoutCI);
        emptyLayoutCI.pBindings = nullptr;
        emptyLayoutCI.bindingCount = 0;
        fatalAssert(
            ownerDevice->vkCreateDescriptorSetLayout(VulkanGraphicsHelper::getDevice(ownerDevice), &emptyLayoutCI, nullptr, &emptyLayout)
                == VK_SUCCESS,
            "%s() : Failed creating empty descriptors set layout", __func__
        );

        VkDescriptorPoolSize poolSize;
        poolSize.descriptorCount = 1;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DESCRIPTOR_POOL_CREATE_INFO(emptyPoolCreateInfo);
        emptyPoolCreateInfo.poolSizeCount = 1;
        emptyPoolCreateInfo.pPoolSizes = &poolSize;
        emptyPoolCreateInfo.maxSets = 1;
        fatalAssert(
            ownerDevice->vkCreateDescriptorPool(VulkanGraphicsHelper::getDevice(ownerDevice), &emptyPoolCreateInfo, nullptr, &emptyPool)
                == VK_SUCCESS,
            "Empty pool creation failed"
        );
        DESCRIPTOR_SET_ALLOCATE_INFO(emptySetAI);
        emptySetAI.descriptorPool = emptyPool;
        emptySetAI.descriptorSetCount = 1;
        emptySetAI.pSetLayouts = &emptyLayout;
        fatalAssert(
            ownerDevice->vkAllocateDescriptorSets(VulkanGraphicsHelper::getDevice(ownerDevice), &emptySetAI, &emptyDescriptor) == VK_SUCCESS,
            "%s() : Failed to allocate empty descriptors set", __func__
        );
    }
}

VulkanDescriptorsSetAllocator::~VulkanDescriptorsSetAllocator()
{
    VkDevice device = VulkanGraphicsHelper::getDevice(ownerDevice);
    // ownerDevice->vkDestroyDescriptorPool(device, globalPool.pool, nullptr);
    // globalPool.allocatedSets.clear();
    // globalPool.availableSets.clear();
    // globalPool.pool = nullptr;
    // globalPool.typeCountMap.clear();

    for (auto allocationPoolsItr = availablePools.begin(); allocationPoolsItr != availablePools.end(); ++allocationPoolsItr)
    {
        for (VulkanDescriptorsSetAllocatorInfo &allocationPool : allocationPoolsItr->second)
        {
            ownerDevice->vkDestroyDescriptorPool(device, allocationPool.pool, nullptr);
        }
    }
    availablePools.clear();

    ownerDevice->vkDestroyDescriptorPool(device, emptyPool, nullptr);
    ownerDevice->vkDestroyDescriptorSetLayout(device, emptyLayout, nullptr);
    emptyLayout = nullptr;
}

VkDescriptorSet
    VulkanDescriptorsSetAllocator::allocDescriptorsSet(const DescriptorsSetQuery &query, const VkDescriptorSetLayout &descriptorsSetLayout)
{
    // Empty
    if (query.supportedTypes.empty())
    {
        return emptyDescriptor;
    }

    std::vector<VkDescriptorSet> chooseSets;
    // if (!query.bHasBindless && isSupportedPool(chooseSets, globalPool, query, 1))
    //{
    //     if (chooseSets.empty())
    //     {
    //         LOG_DEBUG("DescriptorsSetAllocator", "Allocating set from global descriptors set pool");
    //         chooseSets.push_back(allocateSetFromPool(globalPool, descriptorsSetLayout));
    //         globalPool.allocatedSets[chooseSets[0]] = query;
    //     }
    //     else
    //     {
    //         LOG_DEBUG("DescriptorsSetAllocator", "Fetching from available sets of global descriptors
    //         set pool"); globalPool.availableSets.erase(chooseSets[0]);
    //     }
    // }

    if (chooseSets.empty())
    {
        VulkanDescriptorsSetAllocatorInfo &allocationPool = findOrCreateAllocPool(chooseSets, query, 1);

        if (chooseSets.empty())
        {
            chooseSets.push_back(allocateSetFromPool(allocationPool, descriptorsSetLayout));
            allocationPool.allocatedSets[chooseSets[0]] = query;
        }
    }
    return chooseSets[0];
}

bool VulkanDescriptorsSetAllocator::allocDescriptorsSets(
    std::vector<VkDescriptorSet> &sets, const DescriptorsSetQuery &query, const std::vector<VkDescriptorSetLayout> &layouts
)
{
    sets.clear();
    if (query.supportedTypes.empty())
    {
        sets.insert(sets.begin(), layouts.size(), emptyDescriptor);
        return true;
    }
    VulkanDescriptorsSetAllocatorInfo &allocationPool = findOrCreateAllocPool(query, uint32(layouts.size()));

    if (!allocateSetsFromPool(sets, allocationPool, layouts))
    {
        LOG_ERROR("DescriptorsSetAllocator", "%s() : Failed allocating required sets", __func__);
        return false;
    }
    for (VkDescriptorSet &newAllocatedSet : sets)
    {
        allocationPool.allocatedSets[newAllocatedSet] = query;
    }
    return true;
}

bool VulkanDescriptorsSetAllocator::allocDescriptorsSets(
    std::vector<VkDescriptorSet> &sets, const DescriptorsSetQuery &query, const VkDescriptorSetLayout &layout, const uint32 &setsCount
)
{
    sets.clear();
    if (query.supportedTypes.empty())
    {
        sets.insert(sets.begin(), setsCount, emptyDescriptor);
        return true;
    }
    {
        std::vector<VkDescriptorSet> chooseSets;
        VulkanDescriptorsSetAllocatorInfo &allocationPool = findOrCreateAllocPool(chooseSets, query, setsCount);

        if (chooseSets.size() != setsCount)
        {
            uint32 remainingSetsCount = setsCount - uint32(chooseSets.size());
            LOG_DEBUG("DescriptorsSetAllocator", "%s() : Allocating remaining %d required sets", __func__, remainingSetsCount);

            std::vector<VkDescriptorSetLayout> layouts;
            layouts.assign(remainingSetsCount, layout);
            if (!allocateSetsFromPool(sets, allocationPool, layouts))
            {
                LOG_ERROR("DescriptorsSetAllocator", "%s() : Failed allocating required sets", __func__);
                return false;
            }
            for (VkDescriptorSet &newAllocatedSet : sets)
            {
                allocationPool.allocatedSets[newAllocatedSet] = query;
            }
        }
        sets.insert(sets.end(), chooseSets.cbegin(), chooseSets.cend());
    }
    return true;
}

void VulkanDescriptorsSetAllocator::releaseDescriptorsSet(VkDescriptorSet descriptorSet)
{
    // auto descriptorSetItr = globalPool.allocatedSets.find(descriptorSet);

    // if (descriptorSetItr != globalPool.allocatedSets.end())
    //{
    //     globalPool.availableSets.insert(descriptorSet);
    // }
    // else
    if (descriptorSet != emptyDescriptor)
    {
        for (auto allocationPoolsItr = availablePools.begin(); allocationPoolsItr != availablePools.end(); ++allocationPoolsItr)
        {
            for (VulkanDescriptorsSetAllocatorInfo &allocationPool : allocationPoolsItr->second)
            {
                auto allocPoolDescSetItr = allocationPool.allocatedSets.find(descriptorSet);
                if (allocPoolDescSetItr != allocationPool.allocatedSets.end())
                {
                    allocationPool.availableSets.insert(descriptorSet);
                    return;
                }
            }
        }
    }
}

void VulkanDescriptorsSetAllocator::tick(const float &deltaTime)
{
    for (auto allocationPoolsItr = availablePools.begin(); allocationPoolsItr != availablePools.end(); ++allocationPoolsItr)
    {
        for (VulkanDescriptorsSetAllocatorInfo &allocationPool : allocationPoolsItr->second)
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