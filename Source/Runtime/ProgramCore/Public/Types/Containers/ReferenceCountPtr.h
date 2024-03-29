/*!
 * \file ReferenceCountPtr.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <atomic>

#include "Types/CoreDefines.h"
#include "Types/HashTypes.h"
#include "Types/Platform/PlatformAssertionErrors.h"

template <typename PtrType>
class ReferenceCountPtr
{
private:
    PtrType *refPtr;

public:
    ReferenceCountPtr()
        : refPtr(nullptr)
    {}

    ReferenceCountPtr(PtrType *ptr)
    {
        refPtr = ptr;
        if (refPtr)
        {
            refPtr->addRef();
        }
    }

    // Explicit copy and move constructor needed as compiler generates them without matching template
    // constructors
    ReferenceCountPtr(ReferenceCountPtr &&refCountPtr)
    {
        refPtr = refCountPtr.reference();
        refCountPtr.refPtr = nullptr;
    }
    ReferenceCountPtr(const ReferenceCountPtr &refCountPtr)
    {
        refPtr = refCountPtr.reference();
        if (refPtr)
        {
            refPtr->addRef();
        }
    }
    template <class InPtrType>
    ReferenceCountPtr(ReferenceCountPtr<InPtrType> &&refCountPtr)
    {
        refPtr = static_cast<PtrType *>(refCountPtr.reference());
        refCountPtr.detachRef();
    }
    template <class InPtrType>
    ReferenceCountPtr(const ReferenceCountPtr<InPtrType> &refCountPtr)
    {
        refPtr = static_cast<PtrType *>(refCountPtr.reference());
        if (refPtr)
        {
            refPtr->addRef();
        }
    }

    ~ReferenceCountPtr()
    {
        if (refPtr)
        {
            refPtr->removeRef();
            refPtr = nullptr;
        }
    }

    ReferenceCountPtr &operator= (PtrType *ptr)
    {
        PtrType *oldRef = refPtr;
        refPtr = ptr;
        if (refPtr)
        {
            refPtr->addRef();
        }
        if (oldRef)
        {
            oldRef->removeRef();
        }

        return *this;
    }

    // Explicit copy and move assignment needed as compiler generates them without matching template
    // assignments
    FORCE_INLINE ReferenceCountPtr &operator= (ReferenceCountPtr &&refCountPtr)
    {
        if (this != &refCountPtr)
        {
            PtrType *oldRef = refPtr;

            refPtr = refCountPtr.reference();
            if (oldRef)
            {
                oldRef->removeRef();
            }
            refCountPtr.refPtr = nullptr;
        }
        return *this;
    }
    FORCE_INLINE ReferenceCountPtr &operator= (const ReferenceCountPtr &refCountPtr) { return *this = refCountPtr.reference(); }
    template <class InPtrType>
    FORCE_INLINE ReferenceCountPtr &operator= (ReferenceCountPtr<InPtrType> &&refCountPtr)
    {
        if (this != &refCountPtr)
        {
            PtrType *oldRef = refPtr;

            refPtr = refCountPtr.reference();
            if (oldRef)
            {
                oldRef->removeRef();
            }
            refCountPtr.refPtr = nullptr;
        }
        return *this;
    }
    template <class InPtrType>
    FORCE_INLINE ReferenceCountPtr &operator= (const ReferenceCountPtr<InPtrType> &refCountPtr)
    {
        return *this = refCountPtr.reference();
    }

    FORCE_INLINE PtrType *operator->() const { return refPtr; }

    FORCE_INLINE bool operator!= (const ReferenceCountPtr &rhs) const { return reference() != rhs.reference(); }
    template <typename RefType>
    FORCE_INLINE bool operator!= (const ReferenceCountPtr<RefType> &rhs) const
    {
        return reference() != rhs.reference();
    }
    FORCE_INLINE bool operator== (const ReferenceCountPtr &rhs) const { return reference() == rhs.reference(); }
    template <typename RefType>
    FORCE_INLINE bool operator== (const ReferenceCountPtr<RefType> &rhs) const
    {
        return reference() == rhs.reference();
    }
    template <typename RefType>
    FORCE_INLINE bool operator== (RefType *rhs) const
    {
        return reference() == rhs;
    }

    FORCE_INLINE bool operator< (const ReferenceCountPtr &rhs) const { return this->reference() < rhs.reference(); }
    template <typename RefType>
    FORCE_INLINE bool operator< (const ReferenceCountPtr<RefType> &rhs) const
    {
        return this->reference() < rhs.reference();
    }

    FORCE_INLINE PtrType **ptrToReference() const { return &refPtr; }

    FORCE_INLINE PtrType *reference() const { return refPtr; }
    template <typename AsType>
    FORCE_INLINE AsType *reference() const
    {
        return static_cast<AsType *>(refPtr);
    }
    // For compliance with SharedPtr
    FORCE_INLINE PtrType *get() const { return refPtr; }

    FORCE_INLINE bool isValid() const { return refPtr != nullptr; }
    FORCE_INLINE explicit operator bool () const { return refPtr != nullptr; }

    FORCE_INLINE uint32 refCount() const
    {
        uint32 cnt = 0;
        if (refPtr)
        {
            cnt = refPtr->refCount();
            debugAssert(cnt > 0);
        }
        return cnt;
    }

    FORCE_INLINE void swap(ReferenceCountPtr<PtrType> &refCountPtr)
    {
        PtrType *oldRef = refPtr;
        refPtr = refCountPtr.refPtr;
        refCountPtr.refPtr = oldRef;
    }

    FORCE_INLINE void reset() { (*this) = nullptr; }

    // Detaches current ref counted resource without decrementing ref counter, Do not use it
    FORCE_INLINE void detachRef() { refPtr = nullptr; }
};

template <typename RefType, typename ReferencedType>
FORCE_INLINE bool operator== (RefType *lhs, const ReferenceCountPtr<ReferencedType> &rhs)
{
    return lhs == rhs.reference();
}

template <typename RefType>
struct std::hash<ReferenceCountPtr<RefType>>
{
    NODISCARD size_t operator() (const ReferenceCountPtr<RefType> &refCountPtr) const noexcept
    {
        return HashUtility::hash(refCountPtr.reference());
    }
};

/**
 * Memory order safe
 */
class RefCountable
{
private:
    std::atomic<uint32> refCounter;

public:
    virtual ~RefCountable() = default;

    /* ReferenceCountPtr implementation */
    void addRef() { refCounter.fetch_add(1, std::memory_order::release); }

    void removeRef()
    {
        uint32 count = refCounter.fetch_sub(1, std::memory_order::acq_rel);
        if (count == 1)
        {
            delete this;
        }
    }

    uint32 refCount() const { return refCounter.load(std::memory_order::acquire); }

    /* end overrides */
    template <typename AsType>
    ReferenceCountPtr<AsType> makeRefCounted()
    {
        return ReferenceCountPtr<AsType>(this);
    }
};

template <typename RefType>
class RefCountableAs : public RefCountable
{
public:
    virtual const RefType *reference() const { return static_cast<const RefType *>(this); }
    virtual RefType *reference() { return static_cast<RefType *>(this); }
};
