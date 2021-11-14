#pragma once

#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/CoreDefines.h"
#include "Types/HashTypes.h"

template <typename PtrType>
class ReferenceCountPtr
{
private:
    PtrType* refPtr;
public:
    ReferenceCountPtr()
        : refPtr(nullptr)
    {}

    ReferenceCountPtr(PtrType* ptr)
    {
        if ((refPtr = ptr))
        {
            refPtr->addRef();
        }
    }

    // Explicit copy and move constructor needed as compiler generates them without matching template constructors
    ReferenceCountPtr(ReferenceCountPtr&& refCountPtr)
    {
        refPtr = refCountPtr.reference();
        refCountPtr.refPtr = nullptr;
    }
    ReferenceCountPtr(const ReferenceCountPtr& refCountPtr)
    {
        if ((refPtr = refCountPtr.reference()))
        {
            refPtr->addRef();
        }
    }
    template <class InPtrType>
    ReferenceCountPtr(ReferenceCountPtr<InPtrType>&& refCountPtr)
    {
        refPtr = static_cast<PtrType*>(refCountPtr.reference());
        refCountPtr.detachRef();
    }
    template <class InPtrType>
    ReferenceCountPtr(const ReferenceCountPtr<InPtrType>& refCountPtr)
    {
        if ((refPtr = static_cast<PtrType*>(refCountPtr.reference())))
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

        if ((refPtr = ptr))
        {
            refPtr->addRef();
        }
        if (oldRef)
        {
            oldRef->removeRef();
        }

        return *this;
    }

    // Explicit copy and move assignment needed as compiler generates them without matching template assignments
    FORCE_INLINE ReferenceCountPtr& operator=(ReferenceCountPtr&& refCountPtr)
    {
        if (this != &refCountPtr)
        {
            PtrType* oldRef = refPtr;

            refPtr = refCountPtr.reference();
            if (oldRef)
            {
                oldRef->removeRef();
            }
            refCountPtr.refPtr = nullptr;
        }
        return *this;
    }
    FORCE_INLINE ReferenceCountPtr& operator=(const ReferenceCountPtr& refCountPtr)
    {
        return *this = refCountPtr.reference();
    }
    template <class InPtrType>
    FORCE_INLINE ReferenceCountPtr& operator=(ReferenceCountPtr<InPtrType>&& refCountPtr)
    {
        if(this != &refCountPtr)
        {
            PtrType* oldRef = refPtr;

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
    FORCE_INLINE ReferenceCountPtr& operator=(const ReferenceCountPtr<InPtrType>& refCountPtr)
    {
        return *this = refCountPtr.reference();
    }


    FORCE_INLINE PtrType* operator->() const
    {
        return refPtr;
    }

    FORCE_INLINE bool operator!=(const ReferenceCountPtr& rhs) const
    {
        return reference() != rhs.reference();
    }
    template<typename RefType>
    FORCE_INLINE bool operator!=(const ReferenceCountPtr<RefType>& rhs) const
    {
        return reference() != rhs.reference();
    }
    FORCE_INLINE bool operator==(const ReferenceCountPtr& rhs) const
    {
        return reference() == rhs.reference();
    }
    template<typename RefType>
    FORCE_INLINE bool operator==(const ReferenceCountPtr<RefType>& rhs) const
    {
        return reference() == rhs.reference();
    }
    template<typename RefType>
    FORCE_INLINE bool operator==(RefType* rhs) const
    {
        return reference() == rhs;
    }

    FORCE_INLINE bool operator<(const ReferenceCountPtr& rhs) const
    {
        return this->reference() < rhs.reference();
    }
    template<typename RefType>
    FORCE_INLINE bool operator<(const ReferenceCountPtr<RefType>& rhs) const
    {
        return this->reference() < rhs.reference();
    }

    FORCE_INLINE PtrType** ptrToReference() const
    {
        return &refPtr;
    }

    FORCE_INLINE PtrType* reference() const
    {
        return refPtr;
    }
    template <typename AsType>
    FORCE_INLINE AsType* reference() const
    {
        return static_cast<AsType*>(refPtr);
    }
    // For compliance with SharedPtr
    FORCE_INLINE PtrType* get() const
    {
        return refPtr;
    }

    FORCE_INLINE bool isValid() const
    {
        return refPtr != nullptr;
    }
    FORCE_INLINE operator bool() const
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

    // Detaches current ref counted resource without decrementing ref counter, Do not use it
    FORCE_INLINE void detachRef()
    {
        refPtr = nullptr;
    }
};

template<typename RefType, typename ReferencedType>
FORCE_INLINE bool operator==(RefType* lhs, const ReferenceCountPtr<ReferencedType>& rhs)
{
    return lhs == rhs.reference();
}

template <typename RefType>
struct std::hash<ReferenceCountPtr<RefType>>
{
    _NODISCARD size_t operator()(const ReferenceCountPtr<RefType>& refCountPtr) const noexcept
    {
        return HashUtility::hash(refCountPtr.reference());
    }
};