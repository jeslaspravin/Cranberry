/*!
 * \file FreeListAllocator.h
 *
 * \author Jeslas
 * \date September 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Containers/BitArray.h"

/**
 * Free list allocator tracker, does not manages memory itself just manages available pages virtually. Actual memory must be allocated and
 * managed by the user
 */
template <uint32 PageByteSize>
class FreeListAllocTracker
{
public:
    static_assert(Math::isPowOf2(PageByteSize), "Page size must be power of 2");
    constexpr static const uint32 PAGE_SIZE = PageByteSize;
    using SizeType = SizeT;

    using size_type = SizeType;

private:
    // 0bit means available, 1bit means used
    BitArray<SizeType> pageUsage;

public:
    FreeListAllocTracker() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(FreeListAllocTracker)
    FreeListAllocTracker(SizeType byteSize) { resize(byteSize); }

    CONST_EXPR void resize(SizeType byteSize)
    {
        byteSize = Math::alignByUnsafe(byteSize, PAGE_SIZE);
        SizeType pageCount = pageCountFromBytes(byteSize);
        pageUsage.resize(pageCount);
    }

    CONST_EXPR SizeType size() const { return pageUsage.size() * PAGE_SIZE; }
    SizeType getFragmentedSize(SizeType &outFragmentCount) const
    {
        outFragmentCount = 0;
        SizeType foundFragSize = 0, pageIdx = 0, fragmentStartIdx = 0;
        bool bInsideFragment = false;
        for (bool bIsUsed : pageUsage)
        {
            if (bIsUsed)
            {
                if (bInsideFragment)
                {
                    bInsideFragment = false;
                    foundFragSize += pageIdx - fragmentStartIdx;
                }
            }
            else
            {
                if (!bInsideFragment)
                {
                    bInsideFragment = true;
                    fragmentStartIdx = pageIdx;
                    outFragmentCount++;
                }
            }
            pageIdx++;
        }
        // If out of loop while being inside the fragment the last size must be added here
        if (bInsideFragment)
        {
            foundFragSize += pageIdx - fragmentStartIdx;
        }
        return foundFragSize * PAGE_SIZE;
    }
    FORCE_INLINE bool findNextAllocatedBlock(SizeType fromOffsetBytes, SizeType &outOffsetBytes, SizeType &outByteSize) 
    {
        SizeType pageOffset = pageCountFromBytes(fromOffsetBytes);
        debugAssert(pageOffset <= pageUsage.size());

        bool bFound = findAllocatedBlock(pageOffset, outOffsetBytes, outByteSize);
        outOffsetBytes *= PAGE_SIZE;
        outByteSize *= PAGE_SIZE;
        return bFound;
    }

    FORCE_INLINE bool allocate(SizeType byteSize, SizeType alignment, SizeType &outOffsetBytes)
    {
        debugAssert(Math::isAligned(byteSize, PAGE_SIZE) && Math::isAligned(alignment, PAGE_SIZE));
        SizeType pageCount = pageCountFromBytes(byteSize);
        SizeType alignmentCount = Math::max(alignment / PAGE_SIZE, 1);

        SizeType foundAt = 0;
        if (getBestFit(pageCount, alignmentCount, foundAt))
        {
            outOffsetBytes = foundAt * PAGE_SIZE;
            pageUsage.setRange(foundAt, pageCount);
            return true;
        }
        return false;
    }

    FORCE_INLINE void deallocate(SizeType bytesOffset, SizeType byteSize)
    {
        debugAssert(Math::isAligned(bytesOffset, PAGE_SIZE) && Math::isAligned(byteSize, PAGE_SIZE));
        SizeType pageCount = pageCountFromBytes(byteSize);
        SizeType pageIdx = bytesOffset / PAGE_SIZE;
        pageUsage.resetRange(pageIdx, pageCount);
    }

    /* Check if entire range is allocated, Fails even if one page is not allocated */
    FORCE_INLINE bool isRangeAllocated(SizeType bytesOffset, SizeType byteSize) const
    {
        SizeType pageCount = pageCountFromBytes(byteSize);
        SizeType pageIdx = bytesOffset / PAGE_SIZE;
        return pageUsage.checkRange(pageIdx, pageCount, true);
    }
    /* Check if entire range is free, Fails even if one page is allocated */
    FORCE_INLINE bool isRangeFree(SizeType bytesOffset, SizeType byteSize) const
    {
        SizeType pageCount = pageCountFromBytes(byteSize);
        SizeType pageIdx = bytesOffset / PAGE_SIZE;
        return pageUsage.checkRange(pageIdx, pageCount, false);
    }

    // Does not considers alignment, If using this defrag ensure that maximum alignment used is not greater than PAGE_SIZE
    template <typename CallbackFunc>
    void defrag(CallbackFunc &&func)
    {
        auto findBestFitAllocBlock = [this](SizeType count, SizeType startIdx, SizeType &outPageIdx, SizeType &outPageCount) -> bool
        {
            SizeType bestFitCount = 0, foundAtPage = pageUsage.size(), blockStartIdx = startIdx, blockPageCount;

            while (findAllocatedBlock(blockStartIdx, blockStartIdx, blockPageCount))
            {
                if (count >= blockPageCount && (bestFitCount < blockPageCount))
                {
                    bestFitCount = blockPageCount;
                    foundAtPage = blockStartIdx;
                }
                blockStartIdx += blockPageCount;
            }

            if (foundAtPage != pageUsage.size())
            {
                outPageIdx = foundAtPage;
                outPageCount = bestFitCount;
                return true;
            }
            return false;
        };

        SizeType fragPageIdx = 0, fragPageCount;
        while (findAvailableFragment(fragPageIdx, fragPageIdx, fragPageCount))
        {
            SizeType allocPageIdx = 0, allocPageCount;
            if (findBestFitAllocBlock(fragPageCount, fragPageIdx, allocPageIdx, allocPageCount))
            {
                // func(oldByteOffset, newByteOffset, byteSize);
                func(allocPageIdx * PAGE_SIZE, fragPageIdx * PAGE_SIZE, allocPageCount * PAGE_SIZE);
                // Set move range as used and reset old range as available
                pageUsage.setRange(fragPageIdx, allocPageCount);
                pageUsage.resetRange(allocPageIdx, allocPageCount);

                fragPageIdx += allocPageCount;
            }
        }
    }

private:
    FORCE_INLINE SizeType pageCountFromBytes(SizeType byteSize) const { return Math::max(byteSize / PAGE_SIZE, 1); }

    /**
     * Best fit always allocate at the end so there is no need to track last allocation at while first linear allocation.
     * Filling holes will anyway search through the array so no need to track last allocation at there either
     */
    CONST_EXPR bool getBestFit(SizeType count, SizeType alignmentCount, SizeType &outFoundAt) const
    {
        SizeType bestFitCount = pageUsage.size(), foundAtPage = pageUsage.size(), fragmentStartIdx = 0, pageIdx = 0;
        bool bInsideFragment = false;

        for (; pageIdx < pageUsage.size(); pageIdx += alignmentCount)
        {
            // count will always be aligned by alignmentCount, So for a page range to be valid all page in range must be use able
            bool bIsAvailable = pageUsage.checkRange(pageIdx, alignmentCount, false);

            if (bIsAvailable)
            {
                if (!bInsideFragment)
                {
                    bInsideFragment = true;
                    fragmentStartIdx = pageIdx;
                }
            }
            else
            {
                if (bInsideFragment)
                {
                    bInsideFragment = false;
                    SizeType fragSize = pageIdx - fragmentStartIdx;
                    if (fragSize >= count && fragSize <= bestFitCount)
                    {
                        foundAtPage = fragmentStartIdx;
                        bestFitCount = fragSize;
                    }
                }
            }
        }
        // If exited the loop while being inside, Do the fragment fit check here
        if (bInsideFragment)
        {
            SizeType fragSize = pageIdx - fragmentStartIdx;
            if (fragSize >= count && fragSize <= bestFitCount)
            {
                foundAtPage = fragmentStartIdx;
                bestFitCount = fragSize;
            }
        }

        if (foundAtPage < pageUsage.size())
        {
            outFoundAt = foundAtPage + (bestFitCount - count);
            return true;
        }
        return false;
    }

    // Finds first allocated block from startIdx
    bool findAllocatedBlock(SizeType startIdx, SizeType &outPageIdx, SizeType &outPageCount)
    {
        SizeType blockStartIdx = 0, pageIdx = startIdx;
        bool bInsideBlock = false;
        for (; pageIdx != pageUsage.size(); pageIdx++)
        {
            bool bIsUsed = pageUsage[pageIdx];
            if (bIsUsed)
            {
                if (!bInsideBlock)
                {
                    bInsideBlock = true;
                    blockStartIdx = pageIdx;
                }
            }
            else
            {
                if (bInsideBlock)
                {
                    break;
                }
            }
        }

        if (bInsideBlock)
        {
            outPageCount = pageIdx - blockStartIdx;
            outPageIdx = blockStartIdx;
            return true;
        }
        return false;
    }
    // Finds first available fragment block from startIdx
    bool findAvailableFragment(SizeType startIdx, SizeType &outPageIdx, SizeType &outPageCount)
    {
        SizeType blockStartIdx = 0, pageIdx = startIdx;
        bool bInsideFragment = false;
        for (; pageIdx != pageUsage.size(); pageIdx++)
        {
            bool bIsUsed = pageUsage[pageIdx];
            if (bIsUsed)
            {
                if (bInsideFragment)
                {
                    break;
                }
            }
            else
            {
                if (!bInsideFragment)
                {
                    bInsideFragment = true;
                    blockStartIdx = pageIdx;
                }
            }
        }

        if (bInsideFragment)
        {
            outPageCount = pageIdx - blockStartIdx;
            outPageIdx = blockStartIdx;
            return true;
        }
        return false;
    }
};