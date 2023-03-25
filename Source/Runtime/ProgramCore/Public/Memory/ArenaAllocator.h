/*!
 * \file ArenaAllocator.h
 *
 * \author Jeslas
 * \date March 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"
#include "Math/Math.h"
#include "Memory/Memory.h"

#include <list>

class ArenaAllocator
{
public:
    ArenaAllocator(SizeT size)
        : allocSize(size)
        , currentTop(0)
        , currentSize(allocSize)
    {
        currentBlock = (uint8 *)CBEMemory::memAlloc(currentSize);
    }

    ~ArenaAllocator()
    {
        for (const std::pair<SizeT, uint8 *> &dynAlloc : dynUsed)
        {
            CBEMemory::memFree(dynAlloc.second);
        }
        dynUsed.clear();

        if (currentBlock)
        {
            CBEMemory::memFree(currentBlock);
        }
    }

    void *allocate(SizeT bytesCount)
    {
        if (currentTop + bytesCount >= currentSize)
        {
            if (currentBlock)
            {
                dynUsed.push_back({ currentTop, currentBlock });
                currentBlock = nullptr;
                currentTop = 0;
            }

            currentSize = std::max(bytesCount, allocSize);
            currentBlock = (uint8 *)CBEMemory::memAlloc(currentSize);
        }

        void *ptr = currentBlock + currentTop;
        currentTop += bytesCount;
        return ptr;
    }

    void *allocateAligned(SizeT bytesPerElement, SizeT elementsCount, uint32 alignment)
    {
        SizeT bytesCount = bytesPerElement * elementsCount;
        // Allocating the entire alignment size, This is to preserve block's top to be always well aligned(alignment - 1, breaks it when using
        // unaligned alloc)
        bytesCount += alignment;

        void *ptr = allocate(bytesCount);
        UPtrInt intptr = reinterpret_cast<UPtrInt>(ptr);
        intptr = Math::alignBy(intptr, alignment);

        return reinterpret_cast<void *>(intptr);
    }

    template <typename T>
    T *allocate()
    {
        return (T *)allocate(sizeof(T));
    }

    template <typename T>
    T *allocateAligned(SizeT count = 1)
    {
        return reinterpret_cast<T *>(allocateAligned(sizeof(T), count, alignof(T)));
    }

private:
    SizeT allocSize;

    uint8 *currentBlock = nullptr;
    SizeT currentTop = 0;
    SizeT currentSize = 0;
    std::list<std::pair<SizeT, uint8 *>> dynUsed;
};