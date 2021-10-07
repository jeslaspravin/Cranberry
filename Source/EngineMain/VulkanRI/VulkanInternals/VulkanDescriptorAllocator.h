#pragma once

#include "../../Core/Platform/PlatformTypes.h"

#include <vector>
#include <queue>
#include <map>
#include <set>
#include <vulkan_core.h>

class VulkanDevice;
struct DescriptorsSetQuery;

struct DescriptorPoolSizeLessThan {

    bool operator()(const VkDescriptorPoolSize& lhs, const VkDescriptorPoolSize& rhs) const;
};

struct DescriptorsSetQueryLessThan {

    bool recursivelyCompare(std::set<VkDescriptorPoolSize>::const_iterator& lhsItr, std::set<VkDescriptorPoolSize>::const_iterator& lhsEnd
        , std::set<VkDescriptorPoolSize>::const_iterator& rhsItr, std::set<VkDescriptorPoolSize>::const_iterator& rhsEnd) const;

    bool operator()(const DescriptorsSetQuery& lhs, const DescriptorsSetQuery& rhs) const;
};

struct DescriptorsSetQuery
{
    // If used for runtime then pool will be created with Update after bind enabled
    bool bHasBindless = false;
    std::set<VkDescriptorPoolSize,DescriptorPoolSizeLessThan> supportedTypes;
};

struct VulkanDescriptorsSetAllocatorInfo
{
    float idlingDuration;
    uint32 maxSets;

    VkDescriptorPool pool;
    std::set<VkDescriptorSet> availableSets;// availableSets.size() < allocatedCount always , when it become == pool reset and destroy timer begins
    std::map<VkDescriptorSet, DescriptorsSetQuery> allocatedSets;// allocatedCount <= maxSets
    std::map<VkDescriptorType, uint32> typeCountMap;// Maps each type and the maximum count in each type that can be allocated
};


class VulkanDescriptorsSetAllocator
{
private:
    constexpr static uint32 DESCRIPTORS_SET_POOL_MAX_SETS = 20;
    constexpr static uint32 DESCRIPTORS_COUNT_PER_SET = 8;
    const float MAX_IDLING_DURATION = 30;// Duration in seconds after which the descriptor set will be reset(not destroyed)
    VulkanDevice* ownerDevice;
    // Pool that accommodates all the descriptors type and considerable amount of set counts.
    VulkanDescriptorsSetAllocatorInfo globalPool;

    VkDescriptorSetLayout emptyLayout;
    VkDescriptorPool emptyPool;
    VkDescriptorSet emptyDescriptor;

    std::map<DescriptorsSetQuery, std::vector<VulkanDescriptorsSetAllocatorInfo>, DescriptorsSetQueryLessThan> availablePools;
    std::vector<VulkanDescriptorsSetAllocatorInfo*> findInAvailablePool(const DescriptorsSetQuery& query);

    bool isSupportedPool(std::vector<VkDescriptorSet>& availableSets, const VulkanDescriptorsSetAllocatorInfo& allocationPool, const DescriptorsSetQuery& query, const uint32& setsCount) const;
    VkDescriptorSet allocateSetFromPool(VulkanDescriptorsSetAllocatorInfo& allocationPool, const VkDescriptorSetLayout& descriptorsSetLayout) const;
    bool allocateSetsFromPool(std::vector<VkDescriptorSet>& allocatedSets, VulkanDescriptorsSetAllocatorInfo& allocationPool, const std::vector<VkDescriptorSetLayout>& layouts) const;
    VulkanDescriptorsSetAllocatorInfo& createNewPool(const DescriptorsSetQuery& query, const uint32& setsCount, std::vector<VulkanDescriptorsSetAllocatorInfo>& poolGroup) const;
    /* 
    * If allocation pool is found with requested available sets then that allocation pool is returned with number of requested available sets filled.
    * If allocation pools are found with partial number of available sets then those available sets will be filled and a new pool will be created and returned.
    * So good usage will be to check if requested amount of sets are returned and only allocated remaining amount from returned allocation pool.
    */
    VulkanDescriptorsSetAllocatorInfo& findOrCreateAllocPool(std::vector<VkDescriptorSet>& availableSets, const DescriptorsSetQuery& query, const uint32& setsCount);
    /*
    * Similar to above method but never uses available sets.
    * Useful in case of sets with varying layouts
    */
    VulkanDescriptorsSetAllocatorInfo& findOrCreateAllocPool(const DescriptorsSetQuery& query, const uint32& setsCount);
    void resetAllocationPool(VulkanDescriptorsSetAllocatorInfo& allocationPool) const;
public:

    VulkanDescriptorsSetAllocator(VulkanDevice* device);
    ~VulkanDescriptorsSetAllocator();

    VkDescriptorSetLayout getEmptyLayout() const { return emptyLayout; }
    VkDescriptorSet allocDescriptorsSet(const DescriptorsSetQuery& query, const VkDescriptorSetLayout& descriptorsSetLayout);
    // Batch allocate descriptor sets from same pool or from different pool but with same layout, does not allocate from global pool(sets will be resized before allocating)
    bool allocDescriptorsSets(std::vector<VkDescriptorSet>& sets, const DescriptorsSetQuery& query, const std::vector<VkDescriptorSetLayout>& layouts);
    bool allocDescriptorsSets(std::vector<VkDescriptorSet>& sets, const DescriptorsSetQuery& query, const VkDescriptorSetLayout& layout, const uint32& setsCount);
    void releaseDescriptorsSet(VkDescriptorSet descriptorSet);
    // #TODO(Jeslas) : Register for tick 
    void tick(const float& deltaTime);
};
