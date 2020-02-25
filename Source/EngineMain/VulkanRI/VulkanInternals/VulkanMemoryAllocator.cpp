#include "VulkanMemoryAllocator.h"
#include "VulkanDevice.h"
#include "../../Core/Logger/Logger.h"

#include <glm/common.hpp>

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
    VulkanMemoryBlock* freeBlockHead = nullptr;

    uint64 chunkSize;
    VkDeviceMemory deviceMemory;
    // Must be power of 2
    uint64 alignment;

    VulkanMemoryBlock* findAndAlloc(const uint64& blocksCount)
    {
        // OoM
        if (!freeBlockHead)
            return nullptr;

        VulkanMemoryBlock* previousBlock = nullptr;
        VulkanMemoryBlock* currentStartBlock = freeBlockHead;
        VulkanMemoryBlock* currentEndBlock = currentStartBlock->nextFreeBlock;

        VulkanMemoryBlock* tempBlock = currentStartBlock;
        uint64 blockDifference = blocksCount - 1;
        uint64 currentDiff = currentEndBlock ? getBlockIndex(currentEndBlock) - getBlockIndex(currentStartBlock) : 0;
        while(currentEndBlock &&  currentDiff < blockDifference)
        {
            if (getBlockIndex(currentEndBlock) - getBlockIndex(tempBlock) == 1) // Valid chain
            {
                tempBlock = currentEndBlock;
                currentEndBlock = currentEndBlock->nextFreeBlock;
            }
            else
            {
                previousBlock = tempBlock;
                currentStartBlock = tempBlock = currentEndBlock;
                currentEndBlock = currentEndBlock->nextFreeBlock;
            }
            currentDiff = currentEndBlock? getBlockIndex(currentEndBlock) - getBlockIndex(currentStartBlock) : 0;
        }

        if (currentDiff == blockDifference)
        {
            if (previousBlock)
            {
                // Bridge the chain
                previousBlock->nextFreeBlock = currentEndBlock->nextFreeBlock;
            }
            else
            {
                // Reset free head
                freeBlockHead = currentEndBlock->nextFreeBlock;
            }
            return currentStartBlock;
        }
        // OoM
        return nullptr;
    }

	// Must be a valid memoryBlock
	uint64 getBlockIndex(VulkanMemoryBlock* memoryBlock) const
	{
		return memoryBlock->offset / alignment;
	}

public:

    VulkanMemoryChunk(uint64 cSize, VkDeviceMemory dMemory, uint64 blockSize) 
        : chunkSize(0)
        , deviceMemory(dMemory)
        , alignment(blockSize)
    {
        alignSize(cSize, chunkSize);
        blocks.resize(chunkSize / alignment);
        freeBlockHead = blocks.data();
        uint64 currentOffset = 0;
        for (uint64 i = 0; i < blocks.size(); ++i)
        {
            VulkanMemoryBlock& block = blocks[i];
            block.offset = currentOffset;
            block.size = alignment;
            block.deviceMemory = deviceMemory;
            uint64 nextBlockIdx = i + 1;
            if (nextBlockIdx < blocks.size())
            {
                block.nextFreeBlock = &blocks[nextBlockIdx];
            }
            else
            {
                block.nextFreeBlock = nullptr;
            }
            block.free = 1;
			currentOffset += alignment;
            assert(i == getBlockIndex(&block));
        }
    }

    bool isInChunk(VulkanMemoryBlock* memoryBlock) const
    {
        return memoryBlock->deviceMemory == deviceMemory && (memoryBlock->offset <= chunkSize - memoryBlock->size);
    }

    void alignSize(const uint64& size, uint64& alignedSize) const
    {
        // Ensure if it is power of 2
        assert(((alignment - 1) & alignment) == 0);
        alignedSize = (size + alignment - 1) & ~(alignment - 1);
    }
    
    VulkanMemoryBlock* allocateBlock(const uint64& size)
    {
        // Ensure it is properly aligned
        assert(size % alignment == 0);
        uint64 nOfBlocks = size / alignment;

        VulkanMemoryBlock* allocatedBlock = findAndAlloc(nOfBlocks);

        if (allocatedBlock)
        {
            allocatedBlock->size = size;
            uint64 startIndex = getBlockIndex(allocatedBlock);
            for (uint64 idxOffset = 0; idxOffset < nOfBlocks; ++idxOffset)
            {
                allocatedBlock[startIndex + idxOffset].free = 0;
            }
        }
        return allocatedBlock;
    }

    void freeBlock(VulkanMemoryBlock* memoryBlock)
    {
        uint64 nOfBlocks = memoryBlock->size / alignment;
        uint64 startBlockIndex = getBlockIndex(memoryBlock);
        uint64 endBlockIndex = startBlockIndex + nOfBlocks - 1;

        for (uint64 idx = startBlockIndex; idx <= endBlockIndex; ++idx)
        {
            blocks[idx].free = 1;
            blocks[idx].size = alignment;

            if (idx + 1 <= endBlockIndex) // Valid next free block
            {
                blocks[idx].nextFreeBlock = &blocks[idx + 1];
            }
        }

        VulkanMemoryBlock* freeHead = nullptr;
        VulkanMemoryBlock* freeTail = nullptr;
        for (uint64 offsetIdx = 1; offsetIdx <= startBlockIndex; ++offsetIdx)
        {
            VulkanMemoryBlock* block = &blocks[startBlockIndex - offsetIdx];
            if (block->free)
            {
                freeHead = block;
                break;
            }
        }

        if(!freeHead)
		{
			for (uint64 idx = endBlockIndex + 1; idx < blocks.size(); ++idx)
			{
				VulkanMemoryBlock* block = &blocks[idx];
				if (block->free)
				{
					freeTail = block;
					break;
				}
			}
            freeBlockHead = &blocks[startBlockIndex];
		}
        else
        {
            freeTail = freeHead->nextFreeBlock;
            freeHead->nextFreeBlock = &blocks[startBlockIndex];
        }

        if (freeTail)
        {
            blocks[endBlockIndex].nextFreeBlock = freeTail;
        }
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
