#include "VulkanMemoryAllocator.h"
#include "VulkanDevice.h"
#include "../../Core/Logger/Logger.h"

struct VulkanMemoryBlock
{
    uint64 offset;
    // When free size value will be alignment value and when allocated it will be value of its total requested size
    uint64 size;
    VkDeviceMemory deviceMemory;
    VulkanMemoryBlock* nextFreeBlock;
    uint32 free : 1;
};

class VulkanMemoryChunk
{
private:
    std::vector<VulkanMemoryBlock> blocks;
    VulkanMemoryBlock* lastFreeBlock = nullptr;

    uint64 chunkSize;
    VkDeviceMemory deviceMemory;
    // Must be power of 2
    uint64 alignment;

public:

    VulkanMemoryChunk(uint64 cSize, VkDeviceMemory dMemory, uint64 blockSize) 
        : chunkSize(cSize)
        , deviceMemory(dMemory)
        , alignment(blockSize)
    {
        blocks.resize(chunkSize / alignment);
        lastFreeBlock = blocks.data();
        uint64 currentOffset = 0;
        for (VulkanMemoryBlock& block : blocks)
        {
            block.offset = currentOffset;
            block.size = alignment;
            block.deviceMemory = deviceMemory;
            uint64 nextBlockIdx = getBlockIndex(&block) + 1;
            if (nextBlockIdx < blocks.size())
            {
                block.nextFreeBlock = &blocks[nextBlockIdx];
            }
            else
            {
                block.nextFreeBlock = nullptr;
            }
            block.free = 1;
        }
    }

    bool isInChunk(VulkanMemoryBlock* memoryBlock) const
    {
        return memoryBlock->deviceMemory == deviceMemory && (memoryBlock->offset <= chunkSize - memoryBlock->size);
    }

    void alignSize(const uint64& size, uint64& alignedSize) const
    {
        // Ensure if it is power of 2
        assert((alignment - 1) & alignment == 0);
        alignedSize = (size + alignment - 1) & ~(alignment - 1);
    }

    uint64 getBlockIndex(VulkanMemoryBlock* memoryBlock) const
    {
        return memoryBlock->offset / alignment;
    }

    VulkanMemoryBlock* allocateBlock(const uint64& size)
    {
        // Ensure it is properly aligned
        assert(size % alignment == 0 && lastFreeBlock);
        uint32 nOfBlocks = size / alignment;
    }

    void freeBlock(VulkanMemoryBlock* memoryBlock)
    {

    }
};

class VulkanChunkAllocator
{
    // Chunk size
    uint64 cSize;
public:
    VulkanChunkAllocator(uint64 chunkSize) : cSize(chunkSize)
    {

    }

    ~VulkanChunkAllocator()
    {

    }
};

class VulkanMemoryAllocator : public IVulkanMemoryAllocator
{
private:

    VulkanChunkAllocator* chunkAllocators[VK_MAX_MEMORY_HEAPS];

public:
    VulkanMemoryAllocator(VulkanDevice* vDevice) : IVulkanMemoryAllocator(vDevice)
    {}

    void initAllocator() override
    {
        Logger::debug("VulkanMemoryAllocator", "%s()", __func__);
    }


    void destroyAllocator() override
    {
        Logger::debug("VulkanMemoryAllocator", "%s()", __func__);
    }
};

IVulkanMemoryAllocator::IVulkanMemoryAllocator(VulkanDevice* vDevice) : device(vDevice){}

SharedPtr<IVulkanMemoryAllocator> IVulkanMemoryAllocator::createAllocator(VulkanDevice* vDevice)
{
    VulkanMemoryAllocator* allocator = new VulkanMemoryAllocator(vDevice);
    allocator->initAllocator();
    return SharedPtr<IVulkanMemoryAllocator>(allocator);
}
