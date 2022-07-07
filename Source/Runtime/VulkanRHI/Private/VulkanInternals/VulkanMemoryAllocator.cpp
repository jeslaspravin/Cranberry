/*!
 * \file VulkanMemoryAllocator.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/VulkanMemoryAllocator.h"
#include "Logger/Logger.h"
#include "Math/Math.h"
#include "Resources/IVulkanResources.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanMacros.h"

#include <algorithm>
#include <set>

using BlockIdxType = uint32;
struct VulkanMemoryBlock
{
    constexpr static const BlockIdxType INVALID_BLOCK_IDX = 0;
    // with above 0 as invalid index a VulkanMemoryChunk could manage ~maximum of 255GB memory at 64byte alignment
    // Will be actual array index
    BlockIdxType nextFreeIndex = INVALID_BLOCK_IDX;
};

class VulkanMemoryChunk
{
private:
    std::vector<VulkanMemoryBlock> blocks;
    VulkanMemoryBlock *freeBlockHead = nullptr;

    VkDeviceMemory deviceMemory;

    void *mappedMemory;
    uint64 mappedMemRefCounter;

    uint64 cByteSize;
    // Must be power of 2
    uint64 alignment;

public:
    VulkanMemoryChunk(uint64 blockSize)
        : deviceMemory(nullptr)
        , mappedMemory(nullptr)
        , mappedMemRefCounter(0)
        , cByteSize(0)
        , alignment(blockSize)
    {}

    FORCE_INLINE const VulkanMemoryBlock *firstBlock() const { return blocks.data() + 1; }
    FORCE_INLINE VulkanMemoryBlock *firstBlock() { return blocks.data() + 1; }
    FORCE_INLINE BlockIdxType blockIdxToIdx(BlockIdxType blockIdx) const { return blockIdx + 1; }
    FORCE_INLINE BlockIdxType idxToBlockIdx(BlockIdxType idx) const { return idx - 1; }

    FORCE_INLINE bool isInChunk(VulkanMemoryBlock *memoryBlock) const
    {
        return UPtrInt(firstBlock()) <= UPtrInt(memoryBlock) && getArrayIndex(memoryBlock) < blocks.size();
    }

    FORCE_INLINE void alignSize(uint64 size, uint64 &alignedSize) const
    {
        // Ensure if it is power of 2
        debugAssert(Math::isPowOf2(alignment));
        alignedSize = Math::alignBy(size, alignment);
    }
    FORCE_INLINE uint64 alignSize(uint64 size) const
    {
        uint64 alignedSize;
        alignSize(size, alignedSize);
        return alignedSize;
    }

    void setMemory(uint64 chunkSize, VkDeviceMemory dMemory);

    FORCE_INLINE VulkanMemoryBlock *allocateBlock(uint64 size, uint64 offsetAlignment);
    void freeBlock(VulkanMemoryBlock *memoryBlock, uint64 byteSize);
    NODISCARD void *mapMemory(VulkanMemoryBlock *block, VulkanDevice *device);
    void unmapMemory(VulkanMemoryBlock *block, VulkanDevice *device);

    uint64 availableHeapSize() const;
    FORCE_INLINE uint64 chunkSize() const { return cByteSize; }

    FORCE_INLINE VkDeviceMemory getDeviceMemory() const { return deviceMemory; }
    // Must be a valid memoryBlock
    FORCE_INLINE uint64 getBlockIndex(const VulkanMemoryBlock *block) const { return block - firstBlock(); }
    FORCE_INLINE uint64 getBlockByteOffset(const VulkanMemoryBlock *block) const { return alignment * getBlockIndex(block); }

private:
    FORCE_INLINE uint64 getArrayIndex(const VulkanMemoryBlock *block) const { return block - blocks.data(); }
    FORCE_INLINE uint64 getBlockByteOffset(BlockIdxType idx) const { return alignment * idxToBlockIdx(idx); }
    FORCE_INLINE bool isValidBlock(const VulkanMemoryBlock *block) const { return block && block != blocks.data(); }

    VulkanMemoryBlock *findAndAlloc(BlockIdxType blocksCount, uint64 offsetAlignment)
    {
        // OoM
        if (!isValidBlock(freeBlockHead))
            return nullptr;

        if (!Math::isPowOf2(offsetAlignment))
        {
            LOG_WARN(
                "VulkanMemoryAllocator", "Offset alignment %d is not an exponent of 2, \
                 Memory allocator is not developed with that into consideration",
                offsetAlignment
            );
        }

        VulkanMemoryBlock *previousBlock = nullptr;
        BlockIdxType currentStartBlockIdx = getArrayIndex(freeBlockHead);
        BlockIdxType currentEndBlockIdx = blocks[currentStartBlockIdx].nextFreeIndex;

        // TODO(Jeslas) : Ensure if offset alignment is always between min and max value
        // of all min offset alignments.
        bool blocksOffsetAligned = getBlockByteOffset(currentStartBlockIdx) % offsetAlignment == 0;

        BlockIdxType tempBlockIdx = currentStartBlockIdx;
        BlockIdxType currentDiff = 1;
        while (currentEndBlockIdx != VulkanMemoryBlock::INVALID_BLOCK_IDX && currentDiff < blocksCount)
        {
            if (((currentEndBlockIdx - tempBlockIdx) == 1) && blocksOffsetAligned) // Valid chain
            {
                tempBlockIdx = currentEndBlockIdx;
                currentEndBlockIdx = blocks[currentEndBlockIdx].nextFreeIndex;
                currentDiff += 1;
            }
            else
            {
                previousBlock = &blocks[tempBlockIdx];
                currentStartBlockIdx = tempBlockIdx = currentEndBlockIdx;
                currentEndBlockIdx = blocks[currentEndBlockIdx].nextFreeIndex;
                // TODO(Jeslas) : Ensure if offset alignment is always between min and max value
                // of all min offset alignments.
                blocksOffsetAligned = getBlockByteOffset(currentStartBlockIdx) % offsetAlignment == 0;
                currentDiff = 1;
            }
        }

        if (currentDiff == blocksCount)
        {
            if (previousBlock)
            {
                // Bridge the chain
                previousBlock->nextFreeIndex = currentEndBlockIdx;
            }
            else
            {
                // Reset free head
                freeBlockHead = currentEndBlockIdx == VulkanMemoryBlock::INVALID_BLOCK_IDX ? nullptr : &blocks[currentEndBlockIdx];
            }
            return &blocks[currentStartBlockIdx];
        }
        // OoM
        return nullptr;
    }
};

void VulkanMemoryChunk::setMemory(uint64 chunkSize, VkDeviceMemory dMemory)
{
    // Ensure it is properly aligned
    fatalAssertf(chunkSize % alignment == 0, "Chunk memory size is not properly aligned");
    cByteSize = chunkSize;
    deviceMemory = dMemory;

    // +1 since 0 will always be invalid block
    blocks.resize((cByteSize / alignment) + 1);
    freeBlockHead = &blocks[1];
    for (BlockIdxType i = 1; i < blocks.size(); ++i)
    {
        VulkanMemoryBlock &block = blocks[i];
        block.nextFreeIndex = i + 1;
        debugAssert(i == blockIdxToIdx(getBlockIndex(&block)) && i == getArrayIndex(&block));
    }
    blocks[blocks.size() - 1].nextFreeIndex = VulkanMemoryBlock::INVALID_BLOCK_IDX;
}

FORCE_INLINE VulkanMemoryBlock *VulkanMemoryChunk::allocateBlock(uint64 size, uint64 offsetAlignment)
{
    // Ensure it is properly aligned
    fatalAssertf(size % alignment == 0, "Size allocating is not properly aligned");
    BlockIdxType nOfBlocks = size / alignment;

    return findAndAlloc(nOfBlocks, offsetAlignment);
}

void VulkanMemoryChunk::freeBlock(VulkanMemoryBlock *memoryBlock, uint64 byteSize)
{
    BlockIdxType nOfBlocks = byteSize / alignment;
    BlockIdxType firstBlockIndex = getArrayIndex(memoryBlock);
    BlockIdxType lastBlockIndex = firstBlockIndex + nOfBlocks - 1;

    // Not want to set next free for last block here
    for (BlockIdxType idx = firstBlockIndex; idx < lastBlockIndex; ++idx)
    {
        blocks[idx].nextFreeIndex = idx + 1;
    }

    // Happens if we did one large allocation and used everything in it
    if (!isValidBlock(freeBlockHead))
    {
        blocks[lastBlockIndex].nextFreeIndex = VulkanMemoryBlock::INVALID_BLOCK_IDX;
        freeBlockHead = &blocks[firstBlockIndex];
        return;
    }

    BlockIdxType freeHeadIdx = getArrayIndex(freeBlockHead);
    // If freeBlockHead is after last freeing Block then we can just link the end to current
    // freeBlockHead and start as new freeBlockHead
    if (lastBlockIndex < freeHeadIdx)
    {
        blocks[lastBlockIndex].nextFreeIndex = freeHeadIdx;
        freeBlockHead = &blocks[firstBlockIndex];
        return;
    }

    /**
     * Current freeing blocks is in the middle of free list.
     * Find the block before freeing head and after freeing tail and link them together
     */
    BlockIdxType prevLinkIdx = freeHeadIdx;
    while (blocks[prevLinkIdx].nextFreeIndex != VulkanMemoryBlock::INVALID_BLOCK_IDX && blocks[prevLinkIdx].nextFreeIndex < firstBlockIndex)
    {
        prevLinkIdx = blocks[prevLinkIdx].nextFreeIndex;
    }
    // Linking freeing tail to next free and prev free to freeing head
    blocks[lastBlockIndex].nextFreeIndex = blocks[prevLinkIdx].nextFreeIndex;
    blocks[prevLinkIdx].nextFreeIndex = firstBlockIndex;
}

NODISCARD void *VulkanMemoryChunk::mapMemory(VulkanMemoryBlock *block, VulkanDevice *device)
{
    if (mappedMemory == nullptr)
    {
        device->vkMapMemory(VulkanGraphicsHelper::getDevice(device), deviceMemory, 0, cByteSize, 0, &mappedMemory);
    }

    void *outPtr = reinterpret_cast<uint8 *>(mappedMemory) + getBlockByteOffset(block);
    mappedMemRefCounter++;
    return outPtr;
}

void VulkanMemoryChunk::unmapMemory(VulkanMemoryBlock *block, VulkanDevice *device)
{
    mappedMemRefCounter--;
    if (mappedMemRefCounter == 0)
    {
        device->vkUnmapMemory(VulkanGraphicsHelper::getDevice(device), deviceMemory);
        mappedMemory = nullptr;
    }
}

uint64 VulkanMemoryChunk::availableHeapSize() const
{
    uint64 heapSizeLeft = 0;
    if (!isValidBlock(freeBlockHead))
    {
        return heapSizeLeft;
    }
    const VulkanMemoryBlock *nextBlock = freeBlockHead;
    while (isValidBlock(nextBlock))
    {
        heapSizeLeft += alignment;
        nextBlock = &blocks[nextBlock->nextFreeIndex];
    }
    return heapSizeLeft;
}

class VulkanHeapAllocator
{
private:
    // Chunk size
    uint64 cSize;
    uint64 initialAlignment;
    VulkanDevice *device;
    uint32 tIndex;
    uint32 hIndex;

    std::vector<VulkanMemoryChunk *> chunks;
    std::vector<VulkanMemoryChunk *> chunks2xAligned;

public:
    VulkanHeapAllocator(uint64 chunkSize, uint64 alignment, VulkanDevice *vDevice, uint32 typeIndex, uint32 heapIndex)
        : cSize(chunkSize)
        , initialAlignment(alignment)
        , device(vDevice)
        , tIndex(typeIndex)
        , hIndex(heapIndex)
    {
        uint64 currentUsageSize;
        uint64 totalHeapSize;
        device->getMemoryStat(totalHeapSize, currentUsageSize, hIndex);
        cSize = (uint64)(Math::min<uint64>(2 * cSize, totalHeapSize) * 0.5f);
        /* Allocate as required as even 100MB in graphics memory is important */
        // allocateNewChunk(chunks, alignment, cSize);
        // allocateNewChunk(chunks2xAligned, alignment * 2, cSize);
    }

    ~VulkanHeapAllocator()
    {
        for (VulkanMemoryChunk *chunk : chunks)
        {
            device->vkFreeMemory(VulkanGraphicsHelper::getDevice(device), chunk->getDeviceMemory(), nullptr);
            delete chunk;
        }
        for (VulkanMemoryChunk *chunk : chunks2xAligned)
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
        for (const VulkanMemoryChunk *chunk : chunks)
        {
            totalSize += chunk->chunkSize();
        }
        for (const VulkanMemoryChunk *chunk : chunks2xAligned)
        {
            totalSize += chunk->chunkSize();
        }
        return totalSize;
    }

    VulkanMemoryAllocation allocate(const uint64 &size, const uint64 &offsetAlignment)
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

        // Chunks to min wastage after alignment pair
        std::vector<std::pair<std::vector<VulkanMemoryChunk *> *, uint64>> sortedChunks;
        {
            uint64 aligned;
            uint64 aligned2x;
            chunks[0]->alignSize(size, aligned);
            chunks2xAligned[0]->alignSize(size, aligned2x);

            sortedChunks.push_back({ &chunks, aligned - size });
            sortedChunks.push_back({ &chunks2xAligned, aligned2x - size });
            std::sort(
                sortedChunks.begin(), sortedChunks.end(),
                [](std::pair<std::vector<VulkanMemoryChunk *> *, uint64> &lhs, std::pair<std::vector<VulkanMemoryChunk *> *, uint64> &rhs)
                { return lhs.second < rhs.second; }
            );
        }

        for (auto &chunks : sortedChunks)
        {
            uint64 alignedSize = size + chunks.second;

            for (int32 index = (int32)chunks.first->size() - 1; index >= 0; --index)
            {
                VulkanMemoryChunk *chunk = (*chunks.first)[index];
                VulkanMemoryBlock *allocatedBlock = chunk->allocateBlock(alignedSize, offsetAlignment);
                if (allocatedBlock)
                {
                    VulkanMemoryAllocation allocation;
                    allocation.deviceMemory = chunk->getDeviceMemory();
                    allocation.memBlock = allocatedBlock;
                    allocation.byteSize = alignedSize;
                    allocation.byteOffset = chunk->getBlockByteOffset(allocatedBlock);
                    return allocation;
                }
            }
        }

        for (auto &chunks : sortedChunks)
        {
            uint64 alignedSize = size + chunks.second;
            uint64 alignment;
            (*chunks.first)[0]->alignSize(1, alignment);
            // In case if requested size is greater then allocate requested amount
            int32 index = allocateNewChunk(*chunks.first, alignment, Math::max(cSize, alignedSize));

            if (index < 0)
            {
                continue;
            }
            VulkanMemoryChunk *chunk = (*chunks.first)[index];
            VulkanMemoryBlock *allocatedBlock = chunk->allocateBlock(alignedSize, offsetAlignment);
            if (allocatedBlock)
            {
                VulkanMemoryAllocation allocation;
                allocation.deviceMemory = chunk->getDeviceMemory();
                allocation.memBlock = allocatedBlock;
                allocation.byteSize = alignedSize;
                allocation.byteOffset = chunk->getBlockByteOffset(allocatedBlock);
                return allocation;
            }
        }
        return {};
    }

    bool mapMemory(VulkanMemoryAllocation &allocation)
    {
        if (VulkanMemoryChunk *chunk = findBlockChunk(allocation.memBlock))
        {
            allocation.mappedMemory = chunk->mapMemory(allocation.memBlock, device);
            return true;
        }
        return false;
    }

    bool unmapMemory(VulkanMemoryAllocation &allocation)
    {
        if (VulkanMemoryChunk *chunk = findBlockChunk(allocation.memBlock))
        {
            chunk->unmapMemory(allocation.memBlock, device);
            allocation.mappedMemory = nullptr;
            return true;
        }
        return false;
    }

    // return true if removed from this allocator
    bool free(const VulkanMemoryAllocation &allocation)
    {
        if (VulkanMemoryChunk *chunk = findBlockChunk(allocation.memBlock))
        {
            if (allocation.mappedMemory != nullptr)
            {
                chunk->unmapMemory(allocation.memBlock, device);
            }
            chunk->freeBlock(allocation.memBlock, allocation.byteSize);
            return true;
        }
        return false;
    }

private:
    int32 allocateNewChunk(std::vector<VulkanMemoryChunk *> &chunks, uint64 alignment, uint64 chunkSize)
    {
        uint64 currentUsageSize;
        uint64 totalHeapSize;
        device->getMemoryStat(totalHeapSize, currentUsageSize, hIndex);

        uint64 allocatingSize;
        VulkanMemoryChunk *chunk = new VulkanMemoryChunk(alignment);
        chunk->alignSize(chunkSize, allocatingSize);

        if ((totalHeapSize - currentUsageSize) < allocatingSize)
        {
            chunk->alignSize(totalHeapSize - currentUsageSize, allocatingSize);
            allocatingSize -= alignment; // Just to stay in safe limits
        }

        fatalAssertf(allocatingSize != 0, "Out of Memory");

        LOG_DEBUG("VulkanChunkAllocator", "Allocating a chunk of size %d", allocatingSize);

        MEMORY_ALLOCATE_INFO(allocateInfo);
        allocateInfo.allocationSize = allocatingSize;
        allocateInfo.memoryTypeIndex = tIndex;

        VkDeviceMemory memory;
        VkResult result = device->vkAllocateMemory(VulkanGraphicsHelper::getDevice(device), &allocateInfo, nullptr, &memory);

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            LOG_ERROR("VulkanMemory", "Out of Memory");
            return -1;
        }
        else if (result != VK_SUCCESS)
        {
            LOG_ERROR("VulkanMemory", "Allocating memory failed");
            return -1;
        }

        chunk->setMemory(allocatingSize, memory);
        int32 chunkIndex = static_cast<int32>(chunks.size());
        chunks.push_back(chunk);
        return chunkIndex;
    }

    VulkanMemoryChunk *findBlockChunk(VulkanMemoryBlock *block)
    {
        for (VulkanMemoryChunk *chunk : chunks)
        {
            if (chunk->isInChunk(block))
            {
                return chunk;
            }
        }
        for (VulkanMemoryChunk *chunk : chunks2xAligned)
        {
            if (chunk->isInChunk(block))
            {
                return chunk;
            }
        }
        return nullptr;
    }
};

#if DEBUG_BUILD
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
            VulkanMemoryBlock *block1 = c4.allocateBlock(aligned4, 1);
            if (c4.getBlockByteOffset(block1) != 0)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : block offset %d expected "
                    "offset %d",
                    c4.getBlockByteOffset(block1), 0
                );
            }
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", aligned4, c4.availableHeapSize());
            VulkanMemoryBlock *oomBlock = c4.allocateBlock(40, 1);
            if (oomBlock)
            {
                failedAny |= true;
                LOG_ERROR("TestChunk", "unexpected behavior(VulkanMemoryAllocator) : block should be nullptr");
            }
            uint64 aligned28;
            c4.alignSize(27, aligned28);
            VulkanMemoryBlock *block2 = c4.allocateBlock(aligned28, 1);
            if (c4.getBlockByteOffset(block2) != 4)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : block offset %d expected "
                    "offset %d",
                    c4.getBlockByteOffset(block2), 4
                );
            }
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", aligned28, c4.availableHeapSize());
            oomBlock = c4.allocateBlock(4, 1);
            if (oomBlock)
            {
                failedAny |= true;
                LOG_ERROR("TestChunk", "unexpected behavior(VulkanMemoryAllocator) : block should be nullptr");
            }

            // Next free must be 0 as 28bytes of 32byte is allocated and this 4byte free must be the only block(4byte size) free in chunk
            c4.freeBlock(block1, aligned4);
            if (block1->nextFreeIndex != VulkanMemoryBlock::INVALID_BLOCK_IDX)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : Freeing only 1 block of fully allocated chunk, "
                    "Expected next index %d Found next index %d",
                    VulkanMemoryBlock::INVALID_BLOCK_IDX, block1->nextFreeIndex
                );
            }
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", aligned4, c4.availableHeapSize());

            block1 = c4.allocateBlock(aligned4, 1);
            if (c4.getBlockByteOffset(block1) != 0)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : block offset %d expected "
                    "offset %d",
                    c4.getBlockByteOffset(block1), 0
                );
            }
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", aligned4, c4.availableHeapSize());

            c4.freeBlock(block2, aligned28);
            if ((c4.idxToBlockIdx(block2->nextFreeIndex) - c4.getBlockIndex(block2)) != 1)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : Freeing must link free list, Expected next index %d Found next index %d",
                    c4.getBlockIndex(block2) + 1, c4.idxToBlockIdx(block2->nextFreeIndex)
                );
            }
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", aligned28, c4.availableHeapSize());

            block2 = c4.allocateBlock(12, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(12), c4.availableHeapSize());
            VulkanMemoryBlock *block3 = c4.allocateBlock(4, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            VulkanMemoryBlock *block4 = c4.allocateBlock(12, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(12), c4.availableHeapSize());
            c4.freeBlock(block2, c4.alignSize(12));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(12), c4.availableHeapSize());
            c4.freeBlock(block3, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            block2 = c4.allocateBlock(4, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            block3 = c4.allocateBlock(4, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            VulkanMemoryBlock *block5 = c4.allocateBlock(4, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            VulkanMemoryBlock *block6 = c4.allocateBlock(4, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(4), c4.availableHeapSize());

            if (!block2 || !block3 || !block4 || !block6)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk", "unexpected behavior(VulkanMemoryAllocator) : blocks "
                                 "dealloc and realloc failed"
                );
            }

            c4.freeBlock(block2, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            block2 = nullptr;
            c4.freeBlock(block5, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            c4.freeBlock(block6, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            block6 = nullptr;

            block5 = c4.allocateBlock(8, 1);
            if (!block5 || c4.getBlockByteOffset(block5) != 12)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : block offset %d expected "
                    "offset %d",
                    c4.getBlockByteOffset(block5), 12
                );
            }
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(8), c4.availableHeapSize());
            c4.freeBlock(block5, c4.alignSize(8));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(8), c4.availableHeapSize());
            c4.freeBlock(block4, c4.alignSize(12));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(12), c4.availableHeapSize());
            c4.freeBlock(block1, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            c4.freeBlock(block3, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());

            block1 = c4.allocateBlock(4, 1);
            block2 = c4.allocateBlock(4, 1);
            block3 = c4.allocateBlock(4, 1);
            block4 = c4.allocateBlock(4, 1);
            block5 = c4.allocateBlock(4, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", 5 * c4.alignSize(4), c4.availableHeapSize());
            c4.freeBlock(block1, c4.alignSize(4));
            block1 = nullptr;
            c4.freeBlock(block3, c4.alignSize(4));
            block3 = nullptr;
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", 2 * c4.alignSize(4), c4.availableHeapSize());

            block6 = c4.allocateBlock(12, 1);
            LOG_DEBUG("TestChunk", "%d - Allocated %d heap left", c4.alignSize(12), c4.availableHeapSize());
            if (!block6 || c4.getBlockByteOffset(block6) != 20)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : block offset %d expected "
                    "offset %d",
                    c4.getBlockByteOffset(block6), 20
                );
            }

            c4.freeBlock(block2, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            c4.freeBlock(block4, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            c4.freeBlock(block5, c4.alignSize(4));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(4), c4.availableHeapSize());
            c4.freeBlock(block6, c4.alignSize(12));
            LOG_DEBUG("TestChunk", "%d - deallocated %d heap left", c4.alignSize(12), c4.availableHeapSize());
            if (c4.availableHeapSize() != 32)
            {
                failedAny |= true;
                LOG_ERROR(
                    "TestChunk",
                    "unexpected behavior(VulkanMemoryAllocator) : Heap size %d expected size "
                    "%d",
                    c4.availableHeapSize(), 32
                );
            }
        }

        debugAssert(!failedAny);
    }
};
#endif

class VulkanMemoryAllocator : public IVulkanMemoryAllocator
{
private:
    VulkanHeapAllocator *linearChunkAllocators[VK_MAX_MEMORY_TYPES];
    VulkanHeapAllocator *optimalChunkAllocators[VK_MAX_MEMORY_TYPES];
    std::vector<std::pair<uint32, VkMemoryPropertyFlags>> availableMemoryProps;

    void sortAvailableByPriority(bool cpuAccessible);

public:
    VulkanMemoryAllocator(VulkanDevice *vDevice)
        : IVulkanMemoryAllocator(vDevice)
    {}

    void initAllocator() override
    {
        LOG_DEBUG("VulkanMemoryAllocator", "Started");
#if DEBUG_BUILD
        TestChunk::testChunk();
#endif
        // to handle offset alignment
        uint64 alignment = Math::max<uint64>(
            Math::max<uint64>(
                device->properties.limits.minStorageBufferOffsetAlignment, device->properties.limits.minUniformBufferOffsetAlignment
            ),
            device->properties.limits.minTexelBufferOffsetAlignment
        );

        for (uint32 i = 0; i < device->memoryProperties.memoryTypeCount; ++i)
        {
            linearChunkAllocators[i] = nullptr;
            optimalChunkAllocators[i] = nullptr;
            if (device->memoryProperties.memoryTypes[i].propertyFlags != 0)
            {
                // TODO(Jeslas) : Revisit hard coded size per chunk part.
                linearChunkAllocators[i]
                    = new VulkanHeapAllocator(64 * 1024 * 1024, alignment, device, i, device->memoryProperties.memoryTypes[i].heapIndex);

                if (BIT_SET(device->memoryProperties.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                {
                    // If alignment is 64bytes the image memory alignment is considered
                    // 1024bytes = 1KB Maybe reduce this size in future once memory allocator
                    // is improved
                    optimalChunkAllocators[i] = new VulkanHeapAllocator(
                        128 * 1024 * 1024, alignment * 16, device, i, device->memoryProperties.memoryTypes[i].heapIndex
                    );
                }

                /**
                 * index i is needed to check if a resource supports the memory type at index i.
                 * i-th bit from LSB in VkMemoryRequirements.memoryTypeBits will be set if this memory type is supported
                 * Fixed as part of https://github.com/jeslaspravin/Cranberry/commit/c570f2a64e0c6f04c8608028f95e3cc2b40001dc commit
                 */
                availableMemoryProps.push_back({ i, device->memoryProperties.memoryTypes[i].propertyFlags });
            }
        }
    }

    void destroyAllocator() override
    {
        LOG_DEBUG("VulkanMemoryAllocator", "Started");

        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            LOG_DEBUG("VulkanMemoryAllocator", "Freeing %dBytes of linear memory", linearChunkAllocators[indexPropPair.first]->allocatorSize());
            delete linearChunkAllocators[indexPropPair.first];
            linearChunkAllocators[indexPropPair.first] = nullptr;

            if (optimalChunkAllocators[indexPropPair.first] != nullptr)
            {
                LOG_DEBUG(
                    "VulkanMemoryAllocator", "Freeing %dBytes of optimal memory", optimalChunkAllocators[indexPropPair.first]->allocatorSize()
                );
                delete optimalChunkAllocators[indexPropPair.first];
                optimalChunkAllocators[indexPropPair.first] = nullptr;
            }
        }
        availableMemoryProps.clear();
    }

    VulkanMemoryAllocation allocateBuffer(VkBuffer buffer, bool cpuAccessible) override
    {
        VulkanMemoryAllocation allocation;
        VkMemoryRequirements memRequirement;
        device->vkGetBufferMemoryRequirements(device->logicalDevice, buffer, &memRequirement);

        sortAvailableByPriority(cpuAccessible);
        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            uint32 memoryTypeBit = 1u << indexPropPair.first;
            // Cannot use pure device local as CPU accessible or not supported memory type
            if ((cpuAccessible && NO_BITS_SET(indexPropPair.second, ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                || (memoryTypeBit & memRequirement.memoryTypeBits) == 0)
            {
                continue;
            }

            allocation = linearChunkAllocators[indexPropPair.first]->allocate(memRequirement.size, memRequirement.alignment);

            if (allocation.memBlock)
            {
                break;
            }
        }

        return allocation;
    }

    VulkanMemoryAllocation allocateImage(VkImage image, bool cpuAccessible, bool bIsOptimalTiled) override
    {
        VulkanMemoryAllocation allocation;
        VkMemoryRequirements memRequirement;
        device->vkGetImageMemoryRequirements(device->logicalDevice, image, &memRequirement);

        VulkanHeapAllocator **chunkAllocator = bIsOptimalTiled ? optimalChunkAllocators : linearChunkAllocators;
        sortAvailableByPriority(cpuAccessible);
        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            uint32 memoryTypeBit = 1u << indexPropPair.first;
            // Cannot use pure device local as CPU accessible or not supported memory type
            if ((cpuAccessible && NO_BITS_SET(indexPropPair.second, ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                || (memoryTypeBit & memRequirement.memoryTypeBits) == 0)
            {
                continue;
            }

            allocation = chunkAllocator[indexPropPair.first]->allocate(memRequirement.size, memRequirement.alignment);

            if (allocation.memBlock)
            {
                break;
            }
        }

        return allocation;
    }

    void deallocateBuffer(VkBuffer buffer, const VulkanMemoryAllocation &block) override
    {
        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            if (linearChunkAllocators[indexPropPair.first] != nullptr && linearChunkAllocators[indexPropPair.first]->free(block))
            {
                break;
            }
        }
    }

    void deallocateImage(VkImage image, const VulkanMemoryAllocation &block, bool bIsOptimalTiled) override
    {
        VulkanHeapAllocator **chunkAllocator = bIsOptimalTiled ? optimalChunkAllocators : linearChunkAllocators;

        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            if (chunkAllocator[indexPropPair.first] != nullptr && chunkAllocator[indexPropPair.first]->free(block))
            {
                break;
            }
        }
    }

    void mapBuffer(VulkanMemoryAllocation &allocation) override
    {
        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            if (BIT_NOT_SET(indexPropPair.second, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            {
                continue;
            }
            if (linearChunkAllocators[indexPropPair.first]->mapMemory(allocation))
            {
                return;
            }
        }
    }

    void unmapBuffer(VulkanMemoryAllocation &allocation) override
    {
        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            if (BIT_NOT_SET(indexPropPair.second, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            {
                continue;
            }
            if (linearChunkAllocators[indexPropPair.first]->unmapMemory(allocation))
            {
                return;
            }
        }
    }

    void mapImage(VulkanMemoryAllocation &block) override
    {
        VulkanHeapAllocator **chunkAllocator = linearChunkAllocators;

        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            if (BIT_NOT_SET(indexPropPair.second, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            {
                continue;
            }
            if (chunkAllocator[indexPropPair.first]->mapMemory(block))
            {
                return;
            }
        }
    }

    void unmapImage(VulkanMemoryAllocation &block) override
    {
        VulkanHeapAllocator **chunkAllocator = linearChunkAllocators;

        for (const std::pair<const uint32, VkMemoryPropertyFlags> &indexPropPair : availableMemoryProps)
        {
            if (BIT_NOT_SET(indexPropPair.second, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
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
        std::sort(
            availableMemoryProps.begin(), availableMemoryProps.end(),
            [](const std::pair<uint32, VkMemoryPropertyFlags> &lhs, const std::pair<uint32, VkMemoryPropertyFlags> &rhs)
            {
                uint8 lhsScore = ANY_BIT_SET(lhs.second, (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) ? 1 : 0;
                uint8 rhsScore = ANY_BIT_SET(rhs.second, (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) ? 1 : 0;

                if (lhsScore != rhsScore)
                {
                    return lhsScore > rhsScore ? true : false;
                }

                lhsScore = ANY_BIT_SET(lhs.second, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) ? 1 : 0;
                rhsScore = ANY_BIT_SET(rhs.second, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) ? 1 : 0;

                if (lhsScore != rhsScore)
                {
                    return lhsScore > rhsScore ? true : false;
                }
                return (lhs.second & ~VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) < (rhs.second & ~VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            }
        );
    }
    else
    {
        std::sort(
            availableMemoryProps.begin(), availableMemoryProps.end(),
            [](const std::pair<uint32, VkMemoryPropertyFlags> &lhs, const std::pair<uint32, VkMemoryPropertyFlags> &rhs)
            {
                uint8 lhsScore = BIT_SET(lhs.second, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? 1 : 0;
                uint8 rhsScore = BIT_SET(rhs.second, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? 1 : 0;
                ;

                if (lhsScore != rhsScore)
                {
                    return lhsScore > rhsScore ? true : false;
                }

                return (lhs.second & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) < (rhs.second & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            }
        );
    }
}

IVulkanMemoryAllocator::IVulkanMemoryAllocator(VulkanDevice *vDevice)
    : device(vDevice)
{}

SharedPtr<IVulkanMemoryAllocator> IVulkanMemoryAllocator::createAllocator(VulkanDevice *vDevice)
{
    IVulkanMemoryAllocator *allocator = new VulkanMemoryAllocator(vDevice);
    allocator->initAllocator();
    return SharedPtr<IVulkanMemoryAllocator>(allocator, std::default_delete<IVulkanMemoryAllocator>{});
}

/* Resource Implementation */

void IVulkanMemoryResources::setMemoryData(VulkanMemoryAllocation allocation) { memAllocation = allocation; }

uint64 IVulkanMemoryResources::allocatedSize() const { return memAllocation.byteSize; }

uint64 IVulkanMemoryResources::allocationOffset() const { return memAllocation.byteOffset; }

const VulkanMemoryAllocation &IVulkanMemoryResources::getMemoryData() const { return memAllocation; }

VulkanMemoryAllocation &IVulkanMemoryResources::getMemoryData() { return memAllocation; }

VkDeviceMemory IVulkanMemoryResources::getDeviceMemory() const { return memAllocation.deviceMemory; }

void *IVulkanMemoryResources::getMappedMemory() const { return memAllocation.mappedMemory; }
