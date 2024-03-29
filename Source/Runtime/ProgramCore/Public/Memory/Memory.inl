/*!
 * \file Memory.inl
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#ifndef FUNCTION_QUALIFIER
#define FUNCTION_QUALIFIER
#endif

#ifndef INLINE_MEMORY_FUNCS
#define INLINE_MEMORY_FUNCS 0
#endif

#if !INLINE_MEMORY_FUNCS
CBEMemAllocWrapper CBEMemory::GAlloc;
#endif

FUNCTION_QUALIFIER void *CBEMemory::tryMalloc(SizeT size, uint32 alignment /*= CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/) noexcept
{
    return GALLOC->tryMalloc(size, alignment);
}

FUNCTION_QUALIFIER void *CBEMemory::memAlloc(SizeT size, uint32 alignment /*= CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/) noexcept
{
    return GALLOC->memAlloc(size, alignment);
}

FUNCTION_QUALIFIER void *
CBEMemory::tryRealloc(void *currentPtr, SizeT size, uint32 alignment /*= CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/) noexcept
{
    return GALLOC->tryRealloc(currentPtr, size, alignment);
}

FUNCTION_QUALIFIER void *
CBEMemory::memRealloc(void *currentPtr, SizeT size, uint32 alignment /*= CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/) noexcept
{
    return GALLOC->memRealloc(currentPtr, size, alignment);
}

FUNCTION_QUALIFIER void CBEMemory::memFree(void *ptr) noexcept { return GALLOC->memFree(ptr); }

FUNCTION_QUALIFIER SizeT CBEMemory::getAllocationSize(void *ptr) noexcept { return GALLOC->getAllocationSize(ptr); }