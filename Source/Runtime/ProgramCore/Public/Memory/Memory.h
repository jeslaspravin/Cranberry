/*!
 * \file Memory.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Memory/MemAllocator.h"
#include "String/String.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "ProgramCoreExports.h"

#include <new>
#include <source_location>

template <typename MemAllocType, typename AllocatorCreatePolicy>
class MemAllocatorWrapper
{
public:
    using AllocType = MemAllocType;
    using MemAllocTypePtr = MemAllocType*;
    friend AllocatorCreatePolicy;
private:
    MemAllocTypePtr allocator = nullptr;

    FORCE_INLINE void create()
    {
        static bool bSuccess = AllocatorCreatePolicy::create(&allocator);
        fatalAssert(bSuccess, "Failed to create global allocator");
    }
    FORCE_INLINE void destroy()
    {
        AllocatorCreatePolicy::destroy(allocator);
        allocator = nullptr;
    }
public:

    FORCE_INLINE MemAllocTypePtr operator->()
    {
        create();
        fatalAssert(allocator, "Invalid memory allocator!");
        return allocator;
    }
};


struct CBMemAllocCreatePolicy
{
    static bool create(CBMemAlloc** outAllocator);
    static void destroy(CBMemAlloc* allocator);
};

// If want to override to custom memory allocator define CUSTOM_MEM_ALLOCATOR_WRAPPER with allocator type's wrapper
// Allocator must follow same interface as CBMemAlloc
#ifndef CUSTOM_MEM_ALLOCATOR_WRAPPER
#define INLINE_MEMORY_FUNCS 0
#define FUNCTION_SPECIFIER PROGRAMCORE_EXPORT
#define GALLOC CBMemory::GAlloc
using CBMemAllocWrapper = MemAllocatorWrapper<CBMemAlloc, CBMemAllocCreatePolicy>;
#else
#define INLINE_MEMORY_FUNCS 1
#define FUNCTION_SPECIFIER FORCE_INLINE
#define GALLOC CBMemory::GAlloc()
using CBMemAllocWrapper = CUSTOM_MEM_ALLOCATOR_WRAPPER;
#endif


class CBMemory
{
#if !INLINE_MEMORY_FUNCS
public:
    PROGRAMCORE_EXPORT static CBMemAllocWrapper GAlloc;
#else // !INLINE_FMEMORY_OPERATION
private:
    FORCE_INLINE void CBMemAllocWrapper& GAlloc()
    {
        static CBMemAllocWrapper gallocWrapper;
        return gallocWrapper;
    }
#endif // !INLINE_FMEMORY_OPERATION
private:
    CBMemory() = default;
public:

    // To bring all memory copies to single spot
    FORCE_INLINE static void memCopy(void* dstPtr, void* srcPtr, SizeT count)
    {
        memcpy(dstPtr, srcPtr, count);
    }
    FORCE_INLINE static void memMove(void* dstPtr, void* srcPtr, SizeT count)
    {
        memmove(dstPtr, srcPtr, count);
    }
    FORCE_INLINE static void memSet(void* ptr, uint8 value, SizeT count)
    {
        memset(ptr, value, count);
    }
    FORCE_INLINE static void memZero(void* ptr, SizeT count)
    {
        memset(ptr, 0, count);
    }

    FORCE_INLINE static void* builtinMalloc(SizeT size)
    {
        return ::malloc(size);
    }
    FORCE_INLINE static void* builtinRealloc(void* ptr, SizeT size)
    {
        return ::realloc(ptr, size);
    }
    FORCE_INLINE static void builtinFree(void* ptr)
    {
        ::free(ptr);
    }

    FUNCTION_SPECIFIER static void* tryMalloc(SizeT size, uint32 alignment = CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT, std::source_location srcLoc = std::source_location::current());
    FUNCTION_SPECIFIER static void* memAlloc(SizeT size, uint32 alignment = CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT, std::source_location srcLoc = std::source_location::current());
    FUNCTION_SPECIFIER static void* tryRealloc(void* currentPtr, SizeT size, uint32 alignment = CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT, std::source_location srcLoc = std::source_location::current());
    FUNCTION_SPECIFIER static void* memRealloc(void* currentPtr, SizeT size, uint32 alignment = CBMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT, std::source_location srcLoc = std::source_location::current());
    FUNCTION_SPECIFIER static void memFree(void* ptr, std::source_location srcLoc = std::source_location::current());
    FUNCTION_SPECIFIER static SizeT getAllocationSize();
};

#if INLINE_MEMORY_FUNCS
#include "Memory/Memory.inl"
#endif

#undef INLINE_MEMORY_FUNCS
#undef FUNCTION_SPECIFIER


#define CB_NEW_OPERATOR(MemAllocFunc, FuncSpecifier, ...) \
    NODISCARD FuncSpecifier void* operator new (size_t size, __VA_ARGS__) \
    { \
        return MemAllocFunc((SizeT)size); \
    } \
    \
    NODISCARD FuncSpecifier void* operator new[] (size_t size, __VA_ARGS__) \
    { \
        return MemAllocFunc((SizeT)size); \
    } \
    \
    NODISCARD FuncSpecifier void* operator new (size_t size, std::align_val_t alignment, __VA_ARGS__) \
    { \
        return MemAllocFunc((SizeT)size, (uint32)alignment); \
    } \
    \
    NODISCARD FuncSpecifier void* operator new[] (size_t size, std::align_val_t alignment, __VA_ARGS__) \
    { \
        return MemAllocFunc((SizeT)size, (uint32)alignment); \
    }

#define CB_DELETE_OPERATOR(MemAllocFunc, FuncSpecifier, ...) \
    FuncSpecifier void operator delete (void* ptr, __VA_ARGS__) noexcept \
    { \
        MemAllocFunc(ptr); \
    } \
    \
    FuncSpecifier void operator delete[] (void* ptr, __VA_ARGS__) noexcept \
    { \
        MemAllocFunc(ptr); \
    } \
    \
    FuncSpecifier void operator delete (void* ptr, std::align_val_t, __VA_ARGS__) noexcept \
    { \
        MemAllocFunc(ptr); \
    } \
    \
    FuncSpecifier void operator delete[] (void* ptr, std::align_val_t, __VA_ARGS__) noexcept \
    { \
        MemAllocFunc(ptr); \
    }

#define CB_GLOBAL_NEWDELETE_OVERRIDES \
    CB_NEW_OPERATOR(CBMemory::memAlloc,) \
    CB_NEW_OPERATOR(CBMemory::memAlloc,,const std::nothrow_t&) \
    CB_DELETE_OPERATOR(CBMemory::memFree,) \
    CB_DELETE_OPERATOR(CBMemory::memFree,,const std::nothrow_t&)

#define CB_CLASS_NEWDELETE_OVERRIDES(ClassName) \
private: \
    static void* ClassName##_Alloc(SizeT size, uint32 alignment) \
    { \
        return CBMemory::memAlloc(size, alignment); \
    } \
    \
    static void* ClassName##_Alloc(SizeT size) \
    { \
        return CBMemory::memAlloc(size); \
    } \
    \
    static void* ClassName##_Free(void* ptr) \
    { \
        CBMemory::memFree(ptr); \
    } \
public:  \
    CB_NEW_OPERATOR(ClassName##_Alloc, static) \
    CB_NEW_OPERATOR(ClassName##_Alloc, static, const std::nothrow_t&) \
    CB_DELETE_OPERATOR(ClassName##_Free, static) \
    CB_DELETE_OPERATOR(ClassName##_Free, static, const std::nothrow_t&)