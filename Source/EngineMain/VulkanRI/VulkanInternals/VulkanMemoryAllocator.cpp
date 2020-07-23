#include "VulkanMemoryAllocator.h"
#include "VulkanDevice.h"
#include "../../Core/Logger/Logger.h"
#include "../VulkanGraphicsHelper.h"
#include "VulkanMacros.h"
#include "../Resources/IVulkanResources.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

#include <set>
#include <algorithm>
#include "../../Core/Math/Math.h"

struct VulkanMemoryBlock
{
    uint64 offset;
    // When free size value will be alignment value and when allocated it will be value of its total requested size
    uint64 size;
    VkDeviceMemory deviceMemory;
    void* mappedMemory = nullptr;
    VulkanMemoryBlock* nextFreeBlock;
    uint32 free : 1;
};

class VulkanMemoryChunk
{
private:
    std::vector<VulkanMemoryBlock> blocks;
    VulkanMemoryBlock* freeBlockHead = nullptr;

    VkDeviceMemory deviceMemory;

    void* mappedMemory;
    uint64 mappedMemRefCounter;

    uint64 cSize;
    // Must be power of 2
    uint64 alignment;

    VulkanMemoryBlock* findAndAlloc(const uint64& blocksCount, const uint64& offsetAlignment)
    {
        // OoM
        if (!freeBlockHead)
            return nullptr;

        if (!((offsetAlignment - 1) & offsetAlignment) == 0)
        {
            Logger::warn("VulkanMemoryAllocator", "%s() : Offset alignment %d is not an exponent of 2, \
                 Memory allocator is not developed with that into consideration", __func__, offsetAlignment);
        }

        VulkanMemoryBlock* previousBlock = nullptr;
        VulkanMemoryBlock* currentStartBlock = freeBlockHead;
        VulkanMemoryBlock* currentEndBlock = currentStartBlock->nextFreeBlock;

        // TODO(Jeslas) : Ensure if offset alignment is always between min and max value of all min offset alignments.
        bool blocksOffsetAligned = currentStartBlock->offset % offsetAlignment == 0;

        VulkanMemoryBlock* tempBlock = currentStartBlock;
        uint64 currentDiff = 1;
        while(currentEndBlock &&  currentDiff < blocksCount)
        {
            if (getBlockIndex(currentEndBlock) - getBlockIndex(tempBlock) == 1 && blocksOffsetAligned) // Valid chain
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
                // TODO(Jeslas) : Ensure if offset alignment is always between min and max value of all min offset alignments.
                bool blocksOffsetAligned = currentStartBlock->offset % offsetAlignment == 0;
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

    VulkanMemoryChunk(uint64 blockSize) 
        : deviceMemory(nullptr)
        , mappedMemory(nullptr)
        , mappedMemRefCounter(0)
        , cSize(0)
        , alignment(blockSize)
    {}

    void setMemory(uint64 chunkSize, VkDeviceMemory dMemory)
    {
        // Ensure it is properly aligned
        fatalAssert(chunkSize % alignment == 0,"Chunk memory size is not properly aligned");
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
            debugAssert(i == getBlockIndex(&block));
        }
    }

    bool isInChunk(VulkanMemoryBlock* memoryBlock) const
    {
        return memoryBlock->deviceMemory == deviceMemory && (memoryBlock->offset <= cSize - memoryBlock->size);
    }

    void alignSize(const uint64& size, uint64& alignedSize) const
    {
        // Ensure if it is power of 2
        debugAssert(((alignment - 1) & alignment) == 0);
        alignedSize = (size + alignment - 1) & ~(alignment - 1);
    }
    
    VulkanMemoryBlock* allocateBlock(const uint64& size, const uint64& offsetAlignment)
    {
        // Ensure it is properly aligned
        fatalAssert(size % alignment == 0,"Size allocating is not properly aligned");
        uint64 nOfBlocks = size / alignment;

        VulkanMemoryBlock* allocatedBlock = findAndAlloc(nOfBlocks, offsetAlignment);

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

    void mapMemory(VulkanMemoryBlock* block, VulkanDevice* device)
    {
        if (mappedMemory == nullptr)
        {
            device->vkMapMemory(VulkanGraphicsHelper::getDevice(device), deviceMemory, 0, cSize, 0, &mappedMemory);
        }

        block->mappedMemory = reinterpret_cast<uint8*>(mappedMemory) + block->offset;
        mappedMemRefCounter++;
    }

    void unmapMemory(VulkanMemoryBlock* block, VulkanDevice* device)
    {
        block->mappedMemory = nullptr;
        mappedMemRefCounter--;
        if (mappedMemRefCounter == 0)
        {
            device->vkUnmapMemory(VulkanGraphicsHelper::getDevice(device), deviceMemory);
            mappedMemory = nullptr;
        }
    }

    uint64 availableHeapSize() const
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

    uint64 chunkSize() const
    {
        return cSize;
    }

    VkDeviceMemory getDeviceMemory() const
    {
        return deviceMemory;
    }
};

class VulkanChunkAllocator
{
    // Chunk size
    uint64 cSize;
    uint64 initialAlignment;
    VulkanDevice* device;
    uint32 tIndex;
    uint32 hIndex;

    std::vector<VulkanMemoryChunk*> chunks;
    std::vector<VulkanMemoryChunk*> chunks2xAligned;

    int32 allocateNewChunk(std::vector<VulkanMemoryChunk*>& chunks, uint64 alignment, uint64 chunkSize)
    {
        uint64 currentUsageSize;
        uint64 totalHeapSize;
        device->getMemoryStat(totalHeapSize, currentUsageSize, hIndex);

        uint64 allocatingSize;
        VulkanMemoryChunk* chunk = new VulkanMemoryChunk(alignment);
        chunk->alignSize(chunkSize, allocatingSize);

        if (totalHeapSize - currentUsageSize < allocatingSize)
        {
            chunk->alignSize(totalHeapSize - currentUsageSize, allocatingSize);
            allocatingSize -= alignment;// Just to stay in safe limits
        }

        if (allocatingSize == 0)
        {
            Logger::error("VulkanMemory", "%s() : Out of Memory", __func__);
            return -1;
        }
        Logger::debug("VulkanChunkAllocator", "%s() : Allocating a chunk of size %d", __func__, allocatingSize);

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

        chunk->setMemory(allocatingSize, memory);
        int32 chunkIndex = static_cast<int32>(chunks.size());
        chunks.push_back(chunk);
        return chunkIndex;
    }

    VulkanMemoryChunk* findBlockChunk(VulkanMemoryBlock* block)
    {
        for (VulkanMemoryChunk* chunk : chunks)
        {
            if (chunk->isInChunk(block))
            {
                return chunk;
            }
        }
        for (VulkanMemoryChunk* chunk : chunks2xAligned)
        {
            if (chunk->isInChunk(block))
            {
                return chunk;
            }
        }
        return nullptr;
    }

public:
    VulkanChunkAllocator(uint64 chunkSize,uint64 alignment, VulkanDevice* vDevice,uint32 typeIndex,uint32 heapIndex) 
        : cSize(chunkSize)
        , initialAlignment(alignment)
        , device(vDevice)
        , tIndex(typeIndex)
        , hIndex(heapIndex)
    {
        uint64 currentUsageSize;
        uint64 totalHeapSize;
        device->getMemoryStat(totalHeapSize, currentUsageSize, hIndex);
        cSize = (uint64)(Math::min<uint64>(2*cSize, totalHeapSize) * 0.5f);
        /* Allocate as required as even 100MB in graphics memory is important */
        //allocateNewChunk(chunks, alignment, cSize);
        //allocateNewChunk(chunks2xAligned, alignment * 2, cSize);
    }

    ~VulkanChunkAllocator()
    {
        for (VulkanMemoryChunk* chunk : chunks)
        {
            device->vkFreeMemory(VulkanGraphicsHelper::getDevice(device), chunk->getDeviceMemory(), nullptr);
            delete chunk;
        }
        for (VulkanMemoryChunk* chunk : chunks2xAligned)
        {
            device->vkFreeMemory(VulkanGraphicsHelper::getDevice(device), chunk->getDeviceMemory(), nullptr);
            delete chunk;
        }
        chunks.clear();
        chunks2xAligned.clear();
    }

    uint64 allocatorSize() const
    {
        uint64 totalSize = 0;
        for (const VulkanMemoryChunk* chunk : chunks)
        {
            totalSize += chunk->chunkSize();
        }
        for (const VulkanMemoryChunk* chunk : chunks2xAligned)
        {
            totalSize += chunk->chunkSize();
        }
        return totalSize;
    }

    VulkanMemoryBlock* allocate(const uint64& size, const uint64& offsetAlignment)
    {
        // If using chunk allocator for first time, Moved from constructor allocation to here
        if (chunks.empty())
        {
            allocateNewChunk(chunks, initialAlignment, cSize);
        }
        if (chunks2xAligned.empty())
        {
            allocateNewChunk(chunks2xAligned, initialAlignment * 2, cSize);
        }

        VulkanMemoryBlock* allocatedBlock = nullptr;
        // Chunks to min wastage after alignment pair
        std::vector<std::pair<std::vector<VulkanMemoryChunk*>*, uint64>> sortedChunks;
        {
            uint64 aligned;
            uint64 aligned2x;
            chunks[0]->alignSize(size, aligned);
            chunks2xAligned[0]->alignSize(size, aligned2x);

            sortedChunks.push_back({ &chunks,aligned - size });
            sortedChunks.push_back({ &chunks2xAligned,aligned2x - size });
            std::sort(sortedChunks.begin(), sortedChunks.end(), [](std::pair<std::vector<VulkanMemoryChunk*>*, uint64>& lhs,
                std::pair<std::vector<VulkanMemoryChunk*>*, uint64>& rhs)
            {
                return lhs.second < rhs.second;
            });
        }

        for (auto& chunks : sortedChunks)
        {
            uint64 alignedSize = size + chunks.second;
            
            for (int32 index = (int32)chunks.first->size() - 1; index >= 0; --index)
            {
                allocatedBlock = chunks.first->at(index)->allocateBlock(alignedSize, offsetAlignment);
                if (allocatedBlock)
                {
                    return allocatedBlock;
                }
            }
        }

        for (auto& chunks : sortedChunks)
        {
            uint64 alignedSize = size + chunks.second;
            uint64 alignment;
            chunks.first->at(0)->alignSize(1, alignment);
            // In case if requested size is greater then allocate requested amount
            int32 index = allocateNewChunk(*chunks.first, alignment, Math::max(cSize, alignedSize));

            if (index < 0)
            {
                continue;
            }
            allocatedBlock = chunks.first->at(index)->allocateBlock(alignedSize, offsetAlignment);
            if (allocatedBlock)
            {
                break;
            }
        }

        return allocatedBlock;
    }

    bool mapMemory(VulkanMemoryBlock* block)
    {
        if (VulkanMemoryChunk* chunk = findBlockChunk(block))
        {
            chunk->mapMemory(block, device);
            return true;
        }
        return false;
    }

    bool unmapMemory(VulkanMemoryBlock* block)
    {
        if (VulkanMemoryChunk* chunk = findBlockChunk(block))
        {
            chunk->unmapMemory(block, device);
            return true;
        }
        return false;
    }

    // return true if removed from this allocator
    bool free(VulkanMemoryBlock* block)
    {
        if (VulkanMemoryChunk* chunk = findBlockChunk(block))
        {
            if (block->mappedMemory != nullptr)
            {
                chunk->unmapMemory(block, device);
            }
            chunk->freeBlock(block);
            return true;
        }
        return false;
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
            VulkanMemoryBlock* block1 = c4.allocateBlock(aligned4, 1);
            if (block1->offset != 0)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block offset %d expected offset %d"
                    , __func__, block1->offset, 0);
            }
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block1->size, c4.availableHeapSize());
            VulkanMemoryBlock* oomBlock = c4.allocateBlock(40, 1);
            if (oomBlock)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block should be nullptr");
            }
            uint64 aligned28;
            c4.alignSize(27, aligned28);
            VulkanMemoryBlock* block2 = c4.allocateBlock(aligned28, 1);
            if (block2->offset != 4)
            {
                failedAny |= true;
                Logger::error("TestChunk", "%s() : unexpected behavior(VulkanMemoryAllocator) : block offset %d expected offset %d"
                    , __func__, block2->offset, 4);
            }
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block2->size, c4.availableHeapSize());
            oomBlock = c4.allocateBlock(4, 1);
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

            block1 = c4.allocateBlock(aligned4, 1);
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

            block2 = c4.allocateBlock(12, 1);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block2->size, c4.availableHeapSize());
            VulkanMemoryBlock* block3 = c4.allocateBlock(4, 1);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block3->size, c4.availableHeapSize());
            VulkanMemoryBlock* block4 = c4.allocateBlock(12, 1);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block4->size, c4.availableHeapSize());
            c4.freeBlock(block2);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 12, c4.availableHeapSize());
            c4.freeBlock(block3);
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 4, c4.availableHeapSize());
            block2 = c4.allocateBlock(4, 1);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block2->size, c4.availableHeapSize());
            block3 = c4.allocateBlock(4, 1);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block3->size, c4.availableHeapSize());
            VulkanMemoryBlock* block5 = c4.allocateBlock(4, 1);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, block5->size, c4.availableHeapSize());
            VulkanMemoryBlock* block6 = c4.allocateBlock(4, 1);
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

            block5 = c4.allocateBlock(8, 1);
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

            block1 = c4.allocateBlock(4, 1);
            block2 = c4.allocateBlock(4, 1);
            block3 = c4.allocateBlock(4, 1);
            block4 = c4.allocateBlock(4, 1);
            block5 = c4.allocateBlock(4, 1);
            Logger::debug("TestChunk", "%s() : %d - Allocated %d heap left", __func__, 20, c4.availableHeapSize());
            c4.freeBlock(block1);
            block1 = nullptr;
            c4.freeBlock(block3);
            block3 = nullptr;
            Logger::debug("TestChunk", "%s() : %d - deallocated %d heap left", __func__, 8, c4.availableHeapSize());

            block6 = c4.allocateBlock(12, 1);
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

        debugAssert(!failedAny);
    }

};
#endif

class VulkanMemoryAllocator : public IVulkanMemoryAllocator
{
private:

    VulkanChunkAllocator* linearChunkAllocators[VK_MAX_MEMORY_TYPES];
    VulkanChunkAllocator* optimalChunkAllocators[VK_MAX_MEMORY_TYPES];
    std::vector<std::pair<uint32, VkMemoryPropertyFlags>> availableMemoryProps;

    void sortAvailableByPriority(bool cpuAccessible);
public:
    VulkanMemoryAllocator(VulkanDevice* vDevice) : IVulkanMemoryAllocator(vDevice)
    {}

    void initAllocator() override
    {
        Logger::debug("VulkanMemoryAllocator", "%s()", __func__);
#if _DEBUG
        TestChunk::testChunk();
#endif
        // to handle offset alignment
        uint64 alignment = Math::max<uint64>(Math::max<uint64>(device->properties.limits.minStorageBufferOffsetAlignment,
            device->properties.limits.minUniformBufferOffsetAlignment), device->properties.limits.minTexelBufferOffsetAlignment);

        for (uint32 i = 0; i < device->memoryProperties.memoryTypeCount; ++i)
        {
            linearChunkAllocators[i] = nullptr;
            optimalChunkAllocators[i] = nullptr;
            if (device->memoryProperties.memoryTypes[i].propertyFlags != 0)
            {
                // TODO(Jeslas) : Revisit hard coded size per chunk part.
                linearChunkAllocators[i] = new VulkanChunkAllocator(128 * 1024 * 1024, alignment, device, i,
                    device->memoryProperties.memoryTypes[i].heapIndex);

                if((device->memoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) > 0)
                {
                    optimalChunkAllocators[i] = new VulkanChunkAllocator(64 * 1024 * 1024, alignment, device, i,
                        device->memoryProperties.memoryTypes[i].heapIndex);
                }

                availableMemoryProps.push_back({ i,device->memoryProperties.memoryTypes[i].propertyFlags });
            }
        }
    }


    void destroyAllocator() override
    {
        Logger::debug("VulkanMemoryAllocator", "%s()", __func__);

        for (const std::pair<uint32,VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            Logger::debug("VulkanMemoryAllocator", "%s() : Freeing %dBytes of linear memory", __func__, 
                linearChunkAllocators[indexPropPair.first]->allocatorSize());
            delete linearChunkAllocators[indexPropPair.first];
            linearChunkAllocators[indexPropPair.first] = nullptr;

            if (optimalChunkAllocators[indexPropPair.first] != nullptr)
            {
                Logger::debug("VulkanMemoryAllocator", "%s() : Freeing %dBytes of optimal memory", __func__,
                    optimalChunkAllocators[indexPropPair.first]->allocatorSize());
                delete optimalChunkAllocators[indexPropPair.first];
                optimalChunkAllocators[indexPropPair.first] = nullptr;
            }
        }
        availableMemoryProps.clear();
    }

    VulkanMemoryBlock* allocateBuffer(VkBuffer buffer, bool cpuAccessible) override
    {
        VulkanMemoryBlock* block = nullptr;
        VkMemoryRequirements memRequirement;
        device->vkGetBufferMemoryRequirements(device->logicalDevice, buffer, &memRequirement);

        sortAvailableByPriority(cpuAccessible);
        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            uint32 memoryTypeBit = 1u << indexPropPair.first;
            if ((cpuAccessible && (indexPropPair.second & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0)
                || (memoryTypeBit & memRequirement.memoryTypeBits) == 0) // Cannot use pure device local as CPU accessible or not supported memory type
            {
                continue;
            }

            block = linearChunkAllocators[indexPropPair.first]->allocate(memRequirement.size, memRequirement.alignment);

            if (block)
            {
                break;
            }
        }

        return block;
    }

    VulkanMemoryBlock* allocateImage(VkImage image, bool cpuAccessible, bool bIsOptimalTiled) override
    {
        VulkanMemoryBlock* block = nullptr;
        VkMemoryRequirements memRequirement;
        device->vkGetImageMemoryRequirements(device->logicalDevice, image, &memRequirement);

        VulkanChunkAllocator** chunkAllocator = bIsOptimalTiled ? optimalChunkAllocators : linearChunkAllocators;
        sortAvailableByPriority(cpuAccessible);
        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            uint32 memoryTypeBit = 1u << indexPropPair.first;
            if ((cpuAccessible && (indexPropPair.second & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0)
                || (memoryTypeBit & memRequirement.memoryTypeBits) == 0) // Cannot use pure device local as CPU accessible or not supported memory type
            {
                continue;
            }

            block = chunkAllocator[indexPropPair.first]->allocate(memRequirement.size, memRequirement.alignment);

            if (block)
            {
                break;
            }
        }

        return block;
    }

    void deallocateBuffer(VkBuffer buffer, VulkanMemoryBlock* block) override
    {
        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            if (linearChunkAllocators[indexPropPair.first] != nullptr && linearChunkAllocators[indexPropPair.first]->free(block))
            {
                break;
            }
        }
    }

    void deallocateImage(VkImage image, VulkanMemoryBlock* block, bool bIsOptimalTiled) override
    {
        VulkanChunkAllocator** chunkAllocator = bIsOptimalTiled ? optimalChunkAllocators : linearChunkAllocators;

        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            if (chunkAllocator[indexPropPair.first] != nullptr && chunkAllocator[indexPropPair.first]->free(block))
            {
                break;
            }
        }
    }


    void mapBuffer(VulkanMemoryBlock* block) override
    {
        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            if ((indexPropPair.second & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
            {
                continue;
            }
            if (linearChunkAllocators[indexPropPair.first]->mapMemory(block))
            {
                return;
            }
        }
    }


    void unmapBuffer(VulkanMemoryBlock* block) override
    {
        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            if ((indexPropPair.second & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
            {
                continue;
            }
            if (linearChunkAllocators[indexPropPair.first]->unmapMemory(block))
            {
                return;
            }
        }
    }


    void mapImage(VulkanMemoryBlock* block) override
    {
        VulkanChunkAllocator** chunkAllocator = linearChunkAllocators;

        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            if ((indexPropPair.second & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
            {
                continue;
            }
            if (chunkAllocator[indexPropPair.first]->mapMemory(block))
            {
                return;
            }
        }
    }


    void unmapImage(VulkanMemoryBlock* block) override
    {
        VulkanChunkAllocator** chunkAllocator = linearChunkAllocators;

        for (const std::pair<uint32, VkMemoryPropertyFlags>& indexPropPair : availableMemoryProps)
        {
            if ((indexPropPair.second & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
            {
                continue;
            }
            if (chunkAllocator[indexPropPair.first]->unmapMemory(block))
            {
                return;
            }
        }
    }

};

void VulkanMemoryAllocator::sortAvailableByPriority(bool cpuAccessible)
{
    if (cpuAccessible)
    {
        std::sort(availableMemoryProps.begin(), availableMemoryProps.end(),
            [](const std::pair<uint32, VkMemoryPropertyFlags>& lhs, const std::pair<uint32, VkMemoryPropertyFlags>& rhs)
            {
                uint8 lhsScore = (lhs.second & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) > 0 ? 1 : 0;
                uint8 rhsScore = (rhs.second & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) > 0 ? 1 : 0;;

                if (lhsScore != rhsScore)
                {
                    return lhsScore > rhsScore ? true : false;
                }

                lhsScore = (lhs.second & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) > 0 ? 1 : 0;
                rhsScore = (rhs.second & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) > 0 ? 1 : 0;

                if (lhsScore != rhsScore)
                {
                    return lhsScore > rhsScore ? true : false;
                }
                return (lhs.second & ~VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) < (rhs.second & ~VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            });
    }
    else
    {
        std::sort(availableMemoryProps.begin(), availableMemoryProps.end(),
            [](const std::pair<uint32, VkMemoryPropertyFlags>& lhs, const std::pair<uint32, VkMemoryPropertyFlags>& rhs)
            {
                uint8 lhsScore = (lhs.second & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) > 0 ? 1 : 0;
                uint8 rhsScore = (rhs.second & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) > 0 ? 1 : 0;;

                if (lhsScore != rhsScore)
                {
                    return lhsScore > rhsScore ? true : false;
                }

                return (lhs.second & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) < (rhs.second & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            });
    }
}


IVulkanMemoryAllocator::IVulkanMemoryAllocator(VulkanDevice* vDevice) : device(vDevice){}

SharedPtr<IVulkanMemoryAllocator> IVulkanMemoryAllocator::createAllocator(VulkanDevice* vDevice)
{
    IVulkanMemoryAllocator* allocator = new VulkanMemoryAllocator(vDevice);
    allocator->initAllocator();
    return SharedPtr<IVulkanMemoryAllocator>(allocator, std::default_delete<IVulkanMemoryAllocator>{});
}


/* Resource Implementation */

void IVulkanMemoryResources::setMemoryData(VulkanMemoryBlock* block)
{
    blockData = block;
}

uint64 IVulkanMemoryResources::allocatedSize() const
{
    return blockData->size;
}

uint64 IVulkanMemoryResources::allocationOffset() const
{
    return blockData->offset;
}

VulkanMemoryBlock* IVulkanMemoryResources::getMemoryData() const
{
    return blockData;
}

VkDeviceMemory IVulkanMemoryResources::getDeviceMemory() const
{
    return blockData->deviceMemory;
}

void* IVulkanMemoryResources::getMappedMemory() const
{
    return blockData->mappedMemory;
}

