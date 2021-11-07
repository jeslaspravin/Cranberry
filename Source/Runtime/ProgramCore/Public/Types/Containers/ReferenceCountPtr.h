#pragma once

#include "PlatformAssertionErrors.h"
#include "Types/CoreDefines.h"

template <class PtrType>
class ReferenceCountPtr
{
private:
    PtrType* refPtr;
public:

    ReferenceCountPtr(PtrType* ptr)
    {
        if (refPtr = ptr)
        {
            refPtr->addRef();
        }
    }

    template <class InPtrType>
    ReferenceCountPtr(ReferenceCountPtr<InPtrType>&& refCountPtr)
    {
        refPtr = static_cast<PtrType*>(refCountPtr.reference());
        refCountPtr.refPtr = nullptr;
    }

    template <class InPtrType>
    ReferenceCountPtr(const ReferenceCountPtr<InPtrType>& refCountPtr)
    {
        if (refPtr = static_cast<PtrType*>(refCountPtr.reference()))
        {
            refPtr->addRef();
        }
    }

    ~ReferenceCountPtr()
    {
        if (refPtr)
        {
            refPtr->removeRef();
        }
    }

    ReferenceCountPtr& operator=(PtrType* ptr)
    {
        PtrType* oldRef = refPtr;

        if (refPtr = ptr)
        {
            refPtr->addRef();
        }
        if (oldRef)
        {
            refPtr->removeRef();
        }

        return *this;
    }

    template <class InPtrType>
    ReferenceCountPtr& operator=(ReferenceCountPtr<InPtrType>&& refCountPtr)
    {
        if(this != &refCountPtr)
        {
            PtrType* oldRef = refPtr;

            refPtr = ptr;
            if (oldRef)
            {
                refPtr->removeRef();
            }
            refCountPtr.refPtr = nullptr;
        }
        return *this;
    }

    template <class InPtrType>
    ReferenceCountPtr& operator=(const ReferenceCountPtr<InPtrType>& refCountPtr)
    {
        return *this = refCountPtr.reference();
    }

    FORCE_INLINE PtrType* operator->() const
    {
        return refPtr;
    }

    template<typename RefType>
    FORCE_INLINE bool operator==(const ReferenceCountPtr<RefType>& rhs)
    {
        return reference() == rhs.reference();
    }

    template<typename RefType>
    FORCE_INLINE bool operator==(RefType* rhs)
    {
        return reference() == rhs;
    }

    template<typename ReferencedType>
    FORCEINLINE bool operator==(ReferencedType* A, const TRefCountPtr<ReferencedType>& B)
    {
        return A == B.GetReference();
    }

    FORCE_INLINE PtrType** ptrToReference() const
    {
        return &refPtr;
    }

    FORCE_INLINE PtrType* reference() const
    {
        return refPtr;
    }

    FORCE_INLINE bool isValid() const
    {
        return refPtr != nullptr;
    }

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

    FORCE_INLINE void swap(ReferenceCountPtr<PtrType>& refCountPtr)
    {
        PtrType* oldRef = refPtr;
        refPtr = refCountPtr.refPtr;
        refCountPtr.refPtr = oldRef;
    }

    FORCE_INLINE void reset()
    {
        (*this) = nullptr;
    }
};

template<typename RefType, typename ReferencedType>
FORCE_INLINE bool operator==(RefType* lhs, const ReferenceCountPtr<ReferencedType>& rhs)
{
    return lhs == rhs.GetReference();
}