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
#include "ProgramCoreExports.h"
#include "String/String.h"
#include "Types/Platform/PlatformAssertionErrors.h"

template <typename MemAllocType, typename AllocatorCreatePolicy>
class MemAllocatorWrapper
{
public:
    using AllocType = MemAllocType;
    using MemAllocTypePtr = MemAllocType *;
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

struct CBEMemAllocCreatePolicy
{
    static bool create(CBEMemAlloc **outAllocator);
    static void destroy(CBEMemAlloc *allocator);
};

// If want to override to custom memory allocator define CUSTOM_MEM_ALLOCATOR_WRAPPER with allocator
// type's wrapper Allocator must follow same interface as CBEMemAlloc
#ifndef CUSTOM_MEM_ALLOCATOR_WRAPPER
#define INLINE_MEMORY_FUNCS 0
#define FUNCTION_QUALIFIER PROGRAMCORE_EXPORT
#define GALLOC CBEMemory::GAlloc
using CBEMemAllocWrapper = MemAllocatorWrapper<CBEMemAlloc, CBEMemAllocCreatePolicy>;
#else
#define INLINE_MEMORY_FUNCS 1
#define FUNCTION_QUALIFIER FORCE_INLINE
#define GALLOC CBEMemory::GAlloc()
using CBEMemAllocWrapper = CUSTOM_MEM_ALLOCATOR_WRAPPER;
#endif

class CBEMemory
{
#if !INLINE_MEMORY_FUNCS
public:
    PROGRAMCORE_EXPORT static CBEMemAllocWrapper GAlloc;
#else  // !INLINE_FMEMORY_OPERATION
private:
    FORCE_INLINE void CBEMemAllocWrapper &GAlloc()
    {
        static CBEMemAllocWrapper gallocWrapper;
        return gallocWrapper;
    }
#endif // !INLINE_FMEMORY_OPERATION
private:
    CBEMemory() = default;

public:
    // To bring all memory copies to single spot
    FORCE_INLINE static void memCopy(void *dstPtr, const void *srcPtr, SizeT count) { memcpy(dstPtr, srcPtr, count); }
    FORCE_INLINE static void memMove(void *dstPtr, void *srcPtr, SizeT count) { memmove(dstPtr, srcPtr, count); }
    FORCE_INLINE static void memSet(void *ptr, uint8 value, SizeT count) { memset(ptr, value, count); }
    FORCE_INLINE static void memZero(void *ptr, SizeT count) { memset(ptr, 0, count); }

    FORCE_INLINE static void *builtinMalloc(SizeT size) { return ::malloc(size); }
    FORCE_INLINE static void *builtinRealloc(void *ptr, SizeT size) { return ::realloc(ptr, size); }
    FORCE_INLINE static void builtinFree(void *ptr) { ::free(ptr); }

    FUNCTION_QUALIFIER static void *tryMalloc(SizeT size, uint32 alignment = CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT);
    FUNCTION_QUALIFIER static void *memAlloc(SizeT size, uint32 alignment = CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT);
    FUNCTION_QUALIFIER static void *
        tryRealloc(void *currentPtr, SizeT size, uint32 alignment = CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT);
    FUNCTION_QUALIFIER static void *
        memRealloc(void *currentPtr, SizeT size, uint32 alignment = CBEMemAllocWrapper::AllocType::DEFAULT_ALIGNMENT);
    FUNCTION_QUALIFIER static void memFree(void *ptr);
    FUNCTION_QUALIFIER static SizeT getAllocationSize(void *ptr);
};

#if INLINE_MEMORY_FUNCS
#include "Memory/Memory.inl"
#endif

#undef INLINE_MEMORY_FUNCS
#undef FUNCTION_QUALIFIER

#define CBE_NEW_OPERATOR(MemAllocFunc, FuncQual, ...)                                                                                          \
    NODISCARD FuncQual void *operator new(size_t size, __VA_ARGS__) noexcept { return MemAllocFunc((SizeT)size); }                             \
    NODISCARD FuncQual void *operator new[](size_t size, __VA_ARGS__) noexcept { return MemAllocFunc((SizeT)size); }                           \
    NODISCARD FuncQual void *operator new(size_t size, std::align_val_t alignment, __VA_ARGS__) noexcept                                       \
    {                                                                                                                                          \
        return MemAllocFunc((SizeT)size, (uint32)alignment);                                                                                   \
    }                                                                                                                                          \
    NODISCARD FuncQual void *operator new[](size_t size, std::align_val_t alignment, __VA_ARGS__) noexcept                                     \
    {                                                                                                                                          \
        return MemAllocFunc((SizeT)size, (uint32)alignment);                                                                                   \
    }

#define CBE_DELETE_OPERATOR(MemFreeFunc, FuncQual, ...)                                                                                        \
    FuncQual void operator delete(void *ptr, __VA_ARGS__) noexcept { MemFreeFunc(ptr); }                                                       \
    FuncQual void operator delete[](void *ptr, __VA_ARGS__) noexcept { MemFreeFunc(ptr); }                                                     \
    FuncQual void operator delete(void *ptr, std::align_val_t, __VA_ARGS__) noexcept { MemFreeFunc(ptr); }                                     \
    FuncQual void operator delete[](void *ptr, std::align_val_t, __VA_ARGS__) noexcept { MemFreeFunc(ptr); }

#define CBE_NOALLOC_PLACEMENT_NEW_OPERATOR(...)                                                                                                \
    NODISCARD __VA_ARGS__ void *operator new(size_t count, void *allocatedPtr) noexcept { return allocatedPtr; }                               \
    NODISCARD __VA_ARGS__ void *operator new[](size_t count, void *allocatedPtr) noexcept { return allocatedPtr; }                             \
    NODISCARD __VA_ARGS__ void *operator new(size_t count, std::align_val_t al, void *allocatedPtr) noexcept { return allocatedPtr; }          \
    NODISCARD __VA_ARGS__ void *operator new[](size_t count, std::align_val_t al, void *allocatedPtr) noexcept { return allocatedPtr; }

#define CBE_NOALLOC_PLACEMENT_DELETE_OPERATOR(...)                                                                                             \
    /*                                                                                                                                         \
     * No Idea how to delete placement alloc here(It must be handled in its specialization) and we do not allow exception in our code base so  \
     * this should crash the application                                                                                                       \
     */                                                                                                                                        \
    __VA_ARGS__ void operator delete(void *ptr, void *allocatedPtr) noexcept                                                                   \
    {                                                                                                                                          \
        fatalAssert(false, "This placement delete is not meant to be invoked here");                                                           \
    }                                                                                                                                          \
    __VA_ARGS__ void operator delete[](void *ptr, void *allocatedPtr) noexcept                                                                 \
    {                                                                                                                                          \
        fatalAssert(false, "This placement delete is not meant to be invoked here");                                                           \
    }                                                                                                                                          \
    __VA_ARGS__ void operator delete(void *ptr, std::align_val_t al, void *allocatedPtr) noexcept                                              \
    {                                                                                                                                          \
        fatalAssert(false, "This placement delete is not meant to be invoked here");                                                           \
    }                                                                                                                                          \
    __VA_ARGS__ void operator delete[](void *ptr, std::align_val_t al, void *allocatedPtr) noexcept                                            \
    {                                                                                                                                          \
        fatalAssert(false, "This placement delete is not meant to be invoked here");                                                           \
    }

#define CBE_GLOBAL_NEWDELETE_OVERRIDES                                                                                                         \
    CBE_NEW_OPERATOR(CBEMemory::memAlloc, )                                                                                                    \
    CBE_NEW_OPERATOR(CBEMemory::memAlloc, , const std::nothrow_t &)                                                                            \
    CBE_DELETE_OPERATOR(CBEMemory::memFree, )                                                                                                  \
    CBE_DELETE_OPERATOR(CBEMemory::memFree, , const std::nothrow_t &)

#define CBE_CLASS_NEWDELETE_OVERRIDES(ClassName)                                                                                               \
private:                                                                                                                                       \
    static void *ClassName##_Alloc(SizeT size, uint32 alignment) { return CBEMemory::memAlloc(size, alignment); }                              \
                                                                                                                                               \
    static void *ClassName##_Alloc(SizeT size) { return CBEMemory::memAlloc(size); }                                                           \
                                                                                                                                               \
    static void ClassName##_Free(void *ptr) { CBEMemory::memFree(ptr); }                                                                       \
                                                                                                                                               \
public:                                                                                                                                        \
    CBE_NEW_OPERATOR(ClassName##_Alloc, static)                                                                                                \
    CBE_NEW_OPERATOR(ClassName##_Alloc, static, const std::nothrow_t &)                                                                        \
    CBE_NOALLOC_PLACEMENT_NEW_OPERATOR(static)                                                                                                 \
    CBE_DELETE_OPERATOR(ClassName##_Free, static)                                                                                              \
    CBE_DELETE_OPERATOR(ClassName##_Free, static, const std::nothrow_t &)                                                                      \
    CBE_NOALLOC_PLACEMENT_DELETE_OPERATOR(static)
