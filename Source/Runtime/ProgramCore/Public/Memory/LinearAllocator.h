/*!
 * \file LinearAllocator.h
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
 * Linear allocator tracker, does not manages memory itself just manages available pages virtually. Actual memory must be allocated and managed
 * by the user
 */
template <uint32 PageByteSize>
class LinearAllocationTracker
{
public:
    static_assert(Math::isPowOf2(PageByteSize), "Page size must be power of 2");
    constexpr static const uint32 PAGE_SIZE = PageByteSize;
    using SizeType = SizeT;

    using size_type = SizeType;

private:
    BitArray<SizeType> pageAvailability;
    SizeType fragmentCount = 0;

public:
    LinearAllocationTracker() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(LinearAllocationTracker)
    LinearAllocationTracker(SizeType byteSize) { resize(byteSize); }

    CONST_EXPR void resize(SizeType byteSize)
    {
        byteSize = Math::alignByUnsafe(byteSize, PAGE_SIZE);
        pageAvailability.resize(byteSize / PAGE_SIZE);
    }

    CONST_EXPR SizeType size() const { return pageAvailability.size() * PAGE_SIZE; }
    CONST_EXPR SizeType getFragmentCount() const { return fragmentCount; }
    SizeType getFragmentedSize() const
    {
        SizeType foundFragSize = 0, foundFragCount = 0, pageIdx = 0, fragmentStartIdx = 0;
        bool bInsideFragment = false;
        for (bool bIsUsed : pageAvailability)
        {
            if (bIsUsed)
            {
                if (!bInsideFragment)
                {
                    bInsideFragment = true;
                    fragmentStartIdx = pageIdx;
                    foundFragCount++;
                }
            }
            else
            {
                if (bInsideFragment)
                {
                    bInsideFragment = false;
                    foundFragSize += pageIdx - fragmentStartIdx;
                }
            }
            pageIdx++;
        }
        debugAssert(foundFragCount == getFragmentCount());
        return foundFragSize * PAGE_SIZE;
    }

private:
    CONST_EXPR bool getBestFit(SizeType count, SizeType &outFoundAt) const
    {
        SizeType bestFitCount = pageAvailability.size(), foundAtOffset = pageAvailability.size(), pageIdx = 0, fragmentStartIdx = 0;
        bool bInsideFragment = false;
        for (bool bIsUsed : pageAvailability)
        {
            if (bIsUsed)
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
                        foundAtOffset = fragmentStartIdx;
                        bestFitCount = fragSize;
                    }
                }
            }
            pageIdx++;
        }
        if (foundAtOffset < pageAvailability.size())
        {
            outFoundAt = foundAtOffset + (bestFitCount - count);
            return true;
        }
        return false;
    }
};