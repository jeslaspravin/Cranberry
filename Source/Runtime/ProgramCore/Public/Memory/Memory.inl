/*!
 * \file Memory.inl
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#ifndef FUNCTION_SPECIFIER
#define FUNCTION_SPECIFIER
#endif

#ifndef INLINE_MEMORY_FUNCS
#define INLINE_MEMORY_FUNCS 0
#endif

#if !INLINE_MEMORY_FUNCS
CBMemAllocWrapper CBMemory::GAlloc;
#endif

FUNCTION_SPECIFIER  void* CBMemory::tryMalloc(SizeT size
    , uint32 alignment /*= CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/
    , std::source_location srcLoc /*= std::source_location::current()*/)
{
    return GALLOC->tryMalloc(size, alignment);
}

FUNCTION_SPECIFIER void* CBMemory::memAlloc(SizeT size
    , uint32 alignment /*= CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/
    , std::source_location srcLoc /*= std::source_location::current()*/)
{
    return GALLOC->memAlloc(size, alignment);
}

FUNCTION_SPECIFIER void* CBMemory::tryRealloc(void* currentPtr, SizeT size
    , uint32 alignment /*= CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/
    , std::source_location srcLoc /*= std::source_location::current()*/)
{
    return GALLOC->tryRealloc(currentPtr, size, alignment);
}

FUNCTION_SPECIFIER void* CBMemory::memRealloc(void* currentPtr, SizeT size
    , uint32 alignment /*= CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT*/
    , std::source_location srcLoc /*= std::source_location::current()*/)
{
    return GALLOC->memRealloc(currentPtr, size, alignment);
}

FUNCTION_SPECIFIER void CBMemory::memFree(void* ptr, std::source_location srcLoc /*= std::source_location::current()*/)
{
    return GALLOC->memFree(ptr);
}

FUNCTION_SPECIFIER SizeT CBMemory::getAllocationSize()
{
    return GALLOC->getAllocationSize();
}