#include "VulkanMemoryAllocator.h"
#include "VulkanDevice.h"
#include "../../Core/Logger/Logger.h"
#include "../VulkanGraphicsHelper.h"
#include "VulkanMacros.h"

#include <glm/common.hpp>
#include <set>

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

    uint64 cSize;
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
        uint64 currentDiff = 1;
        while(currentEndBlock &&  currentDiff < blocksCount)
        {
            if (getBlockIndex(currentEndBlock) - getBlockIndex(tempBlock) == 1) // Valid chain
            {
                tempBlock = currentEndBlock;
                currentEndBlock = currentEndBlock->nextFreeBlock;
                currentDiff += 1;
            }
            else
            {
                previousBlock = tempBlock;
                currentStartBlock = tempBlock = currentEndBlock;
                currentEndBlock = currentEndBlock->nextFreeBlock;
                currentDiff = 1;
            }
        }

        if (currentDiff == blocksCount)
        {
            if (previousBlock)
            {
                // Bridge the chain
                previousBlock->nextFreeBlock = currentEndBlock;
            }
            else
            {
                // Reset free head
                freeBlockHead = currentEndBlock;
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
    VkDeviceMemory deviceMemory;

    VulkanMemoryChunk(uint64 blockSize) 
        : cSize(0)
        , deviceMemory(nullptr)
        , alignment(blockSize)
    {}

    void setMemory(uint64 chunkSize, VkDeviceMemory dMemory)
    {
        // Ensure it is properly aligned
        assert(chunkSize % alignment == 0);
        cSize = chunkSize;
        deviceMemory = dMemory;

        blocks.resize(cSize / alignment);
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
        return memoryBlock->deviceMemory == deviceMemory && (memoryBlock->offset <= cSize - memoryBlock->size);
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
                blocks[startIndex + idxOffset].free = 0;
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
        else
        {
            blocks[endBlockIndex].nextFreeBlock = nullptr;
        }
    }

    uint64 availableHeapSize()
    {
        uint64 heapSizeLeft = 0;
        if (!freeBlockHead)
        {
            return heapSizeLeft;
        }
        VulkanMemoryBlock* nextBlock = freeBlockHead;
        while (nextBlock)
        {
            heapSizeLeft += alignment;
            nextBlock = nextBlock->nextFreeBlock;
        }
        return heapSizeLeft;
    }

    uint64 chunkSize()
    {
        return cSize;
    }
};

class VulkanChunkAllocator
{
    // Chunk size
    uint64 cSize;
    VulkanDevice* device;
    uint32 tIndex;
    uint32 hIndex;

    std::vector<VulkanMemoryChunk> chunks64;
    std::vector<VulkanMemoryChunk> chunks128;

    int32 allocateNewChunk(std::vector<VulkanMemoryChunk>& chunks, uint64 alignment)
    {
        uint64 currentUsageSize;
        uint64 totalHeapSize;
        device->getMemoryStat(totalHeapSize, currentUsageSize, hIndex);

        uint64 allocatingSize;
        VulkanMemoryChunk chunk = VulkanMemoryChunk(alignment);
        chunk.alignSize(cSize, allocatingSize);

        if (totalHeapSize - currentUsageSize < allocatingSize)
        {
            chunk.alignSize(totalHeapSize - currentUsageSize, allocatingSize);
            allocatingSize -= alignment;// Just to stay in safe limits
        }

        if (allocatingSize == 0)
        {
            Logger::error("VulkanMemory", "%s() : Out of Memory", __func__);
            return -1;
        }
        MEMORY_ALLOCATE_INFO(allocateInfo);
        allocateInfo.allocationSize = allocatingSize;
        allocateInfo.memoryTypeIndex = tIndex;

        VkDeviceMemory memory;
        VkResult result = device->vkAllocateMemory(VulkanGraphicsHelper::getDevice(device), &allocateInfo, nullptr, &memory);

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            Logger::error("VulkanMemory", "%s() : Out of Memory", __func__);
            return -1;
        }
        else if (result != VK_SUCCESS)
        {
            Logger::error("VulkanMemory", "%s() : Allocating memory failed", __func__);
            return -1;
        }
        int32 chunkIndex = (int32)chunks.size();
        chunks.push_back(chunk);
        chunks[chunkIndex].setMemory(allocatingSize, memory);
        return chunkIndex;
    }

public:
    VulkanChunkAllocator(uint64 chunkSize,VulkanDevice* vDevice,uint32 typeIndex,uint32 heapIndex) 
        : cSize(chunkSize)
        , device(vDevice)
        , tIndex(typeIndex)
        , hIndex(heapIndex)
    {
        uint64 currentUsageSize;
        uint64 totalHeapSize;
        device->getMemoryStat(totalHeapSize, currentUsageSize, hIndex);
        cSize = (uint64)(glm::min<uint64>(2*cSize, totalHeapSize) * 0.5f);
        allocateNewChunk(chunks64, 64);
        allocateNewChunk(chunks128, 128);
    }

    ~VulkanChunkAllocator()
    {
        for (VulkanMemoryChunk& chunk : chunks64)
        {
            device->vkFreeMemory(VulkanGraphicsHelper::getDevice(device), chunk.deviceMemory, nullptr);
        }
        for (VulkanMemoryChunk& chunk : chunks128)
        {
            device->vkFreeMemory(VulkanGraphicsHelper::getDevice(device), chunk.deviceMemory, nullptr);
        }
        chunks64.clear();
        chunks128.clear();
    }

    uint64 allocatorSize()
    {
        uint64 totalSize = 0;
        for (VulkanMemoryChunk& chunk : chunks64)
        {
            totalSize += chunk.chunkSize();
        }
        for (VulkanMemoryChunk& chunk : chunks128)
        {
            totalSize += chunk.chunkSize();
        }
        return totalSize;
    }
};

#if _DEBUG
class TestChunk
{
public:
    static void testChunk()
    {
        VulkanMemoryChunk c4 = VulkanMemoryChunk(4);
        c4.setMemory(32, nullptr);
        bool failedAny = false;
        {
            uint64 aligned4;
            c4.alignSize(3, aligned4);
            VulkanMemoryBlock* block1 = c4.allocateBlock(aligned4);
            if (block1->offset != 0)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block offset %d expected offset %d"
                    , __func__, block1->offset, 0);
            }
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block1->size, c4.availableHeapSize());
            VulkanMemoryBlock* oomBlock = c4.allocateBlock(40);
            if (oomBlock)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block should be nullptr");
            }
            uint64 aligned28;
            c4.alignSize(27, aligned28);
            VulkanMemoryBlock* block2 = c4.allocateBlock(aligned28);
            if (block2->offset != 4)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block offset %d expected offset %d"
                    , __func__, block2->offset, 4);
            }
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block2->size, c4.availableHeapSize());
            oomBlock = c4.allocateBlock(4);
            if (oomBlock)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block should be nullptr");
            }

            c4.freeBlock(block1);
            if (block1->free != 1)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block should be free");
            }
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());

            block1 = c4.allocateBlock(aligned4);
            if (block1->offset != 0)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block offset %d expected offset %d"
                    , __func__, block1->offset, 0);
            }
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block1->size, c4.availableHeapSize());

            c4.freeBlock(block2);
            if (block2->free != 1)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block should be free");
            }
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 28, c4.availableHeapSize());

            block2 = c4.allocateBlock(12);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block2->size, c4.availableHeapSize());
            VulkanMemoryBlock* block3 = c4.allocateBlock(4);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block3->size, c4.availableHeapSize());
            VulkanMemoryBlock* block4 = c4.allocateBlock(12);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block4->size, c4.availableHeapSize());
            c4.freeBlock(block2);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 12, c4.availableHeapSize());
            c4.freeBlock(block3);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            block2 = c4.allocateBlock(4);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block2->size, c4.availableHeapSize());
            block3 = c4.allocateBlock(4);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block3->size, c4.availableHeapSize());
            VulkanMemoryBlock* block5 = c4.allocateBlock(4);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block5->size, c4.availableHeapSize());
            VulkanMemoryBlock* block6 = c4.allocateBlock(4);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block6->size, c4.availableHeapSize());

            if (!block2 || !block3 || !block4 || !block6)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : blocks dealloc and realloc failed");
            }

            c4.freeBlock(block2);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            block2 = nullptr;
            c4.freeBlock(block5);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            c4.freeBlock(block6);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            block6 = nullptr;

            block5 = c4.allocateBlock(8);
            if (!block5 || block5->offset != 12)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block offset %d expected offset %d"
                    , __func__, block5->offset, 12);
            }
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block5->size, c4.availableHeapSize());
            c4.freeBlock(block5);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 8, c4.availableHeapSize());
            c4.freeBlock(block4);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 12, c4.availableHeapSize());
            c4.freeBlock(block1);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            c4.freeBlock(block3);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());

            block1 = c4.allocateBlock(4);
            block2 = c4.allocateBlock(4);
            block3 = c4.allocateBlock(4);
            block4 = c4.allocateBlock(4);
            block5 = c4.allocateBlock(4);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, 20, c4.availableHeapSize());
            c4.freeBlock(block1);
            block1 = nullptr;
            c4.freeBlock(block3);
            block3 = nullptr;
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 8, c4.availableHeapSize());

            block6 = c4.allocateBlock(12);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block6->size, c4.availableHeapSize());
            if (!block6 || block6->offset != 20)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block offset %d expected offset %d"
                    , __func__, block6->offset, 20);
            }

            c4.freeBlock(block2);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            c4.freeBlock(block4);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            c4.freeBlock(block5);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            c4.freeBlock(block6);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 12, c4.availableHeapSize());
            if (c4.availableHeapSize() != 32)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : Heap size %d expected size %d"
                    , __func__, c4.availableHeapSize(), 32);
            }
        }

        assert(!failedAny);
    }

};
#endif

class VulkanMemoryAllocator : public IVulkanMemoryAllocator
{
private:

    VulkanChunkAllocator* chunkAllocators[VK_MAX_MEMORY_TYPES];

public:
    VulkanMemoryAllocator(VulkanDevice* vDevice) : IVulkanMemoryAllocator(vDevice)
    {}

    void initAllocator() override
    {
        Logger::debug("VulkanMemoryAllocator", "%s()", __func__);
#if _DEBUG
        TestChunk::testChunk();
#endif
        uint64 totalInitAlloc = 0;
        {
            std::set<uint32> uniqAllocatorCheck;
            for (uint32 i = 0; i < device->memoryProperties.memoryTypeCount; ++i)
            {
                auto insertResult = uniqAllocatorCheck.insert(device->memoryProperties.memoryTypes[i].propertyFlags);
                
                if (insertResult.second && device->memoryProperties.memoryTypes[i].propertyFlags != 0)
                {
                    chunkAllocators[i] = new VulkanChunkAllocator(150 * 1024 * 1024, device, i,
                        device->memoryProperties.memoryTypes[i].heapIndex);
                    totalInitAlloc += chunkAllocators[i]->allocatorSize();
                }
                else
                {
                    chunkAllocators[i] = nullptr;
                }
            }
        }

        Logger::debug("VulkanMemoryAllocator", "%s() : Initial allocation size %dBytes", __func__, totalInitAlloc);
    }


    void destroyAllocator() override
    {
        Logger::debug("VulkanMemoryAllocator", "%s()", __func__);

        for (uint32 i = 0; i < device->memoryProperties.memoryTypeCount; ++i)
        {
            if (chunkAllocators[i])
            {
                Logger::debug("VulkanMemoryAllocator", "%s() : Freeing %dBytes", __func__, chunkAllocators[i]->allocatorSize());
                delete chunkAllocators[i];
                chunkAllocators[i] = nullptr;
            }
        }
    }
};

IVulkanMemoryAllocator::IVulkanMemoryAllocator(VulkanDevice* vDevice) : device(vDevice){}

SharedPtr<IVulkanMemoryAllocator> IVulkanMemoryAllocator::createAllocator(VulkanDevice* vDevice)
{
    VulkanMemoryAllocator* allocator = new VulkanMemoryAllocator(vDevice);
    allocator->initAllocator();
    return SharedPtr<IVulkanMemoryAllocator>(allocator);
}
