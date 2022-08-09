/*!
 * \file ObjectPtrs.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObject.h"
#include "CBEObjectHelpers.h"

namespace cbe
{

template <ObjectType PtrType>
class WeakObjPtr
{
private:
    ObjectAllocIdx allocIdx;
    StringID objectId;

    friend std::hash<WeakObjPtr<PtrType>>;

public:
    WeakObjPtr()
        : allocIdx(0)
        , objectId(StringID::INVALID)
    {}

    WeakObjPtr(PtrType *ptr)
        : allocIdx(0)
        , objectId(StringID::INVALID)
    {
        if (cbe::isValid(ptr))
        {
            allocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(ptr);
            objectId = ptr->getStringID();
        }
    }

    // Explicit copy and move constructor needed as compiler generates them without matching template
    // constructors
    WeakObjPtr(WeakObjPtr &&weakPtr)
        : allocIdx(weakPtr.allocIdx)
        , objectId(weakPtr.objectId)
    {
        weakPtr.detachRef();
    }
    WeakObjPtr(const WeakObjPtr &weakPtr)
        : allocIdx(weakPtr.allocIdx)
        , objectId(weakPtr.objectId)
    {}
    template <class InPtrType>
    WeakObjPtr(WeakObjPtr<InPtrType> &&weakPtr)
    {
        if (PtrType *ptr = weakPtr.getAs<PtrType>())
        {
            WeakObjPtr(ptr);
        }
        weakPtr.detachRef();
    }
    template <class InPtrType>
    WeakObjPtr(const WeakObjPtr<InPtrType> &weakPtr)
    {
        if (PtrType *ptr = weakPtr.getAs<PtrType>())
        {
            WeakObjPtr(ptr);
        }
    }

    ~WeakObjPtr() { reset(); }

    WeakObjPtr &operator=(PtrType *ptr)
    {
        if (cbe::isValid(ptr))
        {
            allocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(ptr);
            objectId = ptr->getStringID();
        }
    }

    // Explicit copy and move assignment needed as compiler generates them without matching template
    // assignments
    FORCE_INLINE WeakObjPtr &operator=(WeakObjPtr &&weakPtr)
    {
        if (this != &weakPtr)
        {
            allocIdx = weakPtr.allocIdx;
            objectId = weakPtr.objectId;
            weakPtr.detachRef();
        }
        return *this;
    }
    FORCE_INLINE WeakObjPtr &operator=(const WeakObjPtr &weakPtr)
    {
        allocIdx = weakPtr.allocIdx;
        objectId = weakPtr.objectId;
    }
    template <class InPtrType>
    FORCE_INLINE WeakObjPtr &operator=(WeakObjPtr<InPtrType> &&weakPtr)
    {
        if (this != &weakPtr)
        {
            if (PtrType *ptr = weakPtr.getAs<PtrType>())
            {
                *this = ptr;
                weakPtr.detachRef();
            }
        }
        return *this;
    }
    template <class InPtrType>
    FORCE_INLINE WeakObjPtr &operator=(const WeakObjPtr<InPtrType> &weakPtr)
    {
        if (PtrType *ptr = weakPtr.getAs<PtrType>())
        {
            *this = ptr;
        }
        return *this;
    }

    FORCE_INLINE PtrType *operator->() const { return get(); }

    FORCE_INLINE bool operator!=(const WeakObjPtr &rhs) const { return get() != rhs.get(); }
    template <typename Type>
    FORCE_INLINE bool operator!=(const WeakObjPtr<Type> &rhs) const
    {
        return get() != rhs.get();
    }
    FORCE_INLINE bool operator==(const WeakObjPtr &rhs) const { return get() == rhs.get(); }
    template <typename Type>
    FORCE_INLINE bool operator==(const WeakObjPtr<Type> &rhs) const
    {
        return get() == rhs.get();
    }
    template <typename Type>
    FORCE_INLINE bool operator==(Type *rhs) const
    {
        return get() == rhs;
    }

    FORCE_INLINE bool operator<(const WeakObjPtr &rhs) const { return get() < rhs.get(); }
    template <typename Type>
    FORCE_INLINE bool operator<(const WeakObjPtr<Type> &rhs) const
    {
        return get() < rhs.get();
    }

    template <typename AsType>
    FORCE_INLINE AsType *getAs() const
    {
        return cast<AsType>(get());
    }
    // For compliance with SharedPtr
    FORCE_INLINE PtrType *get() const
    {
        if (objectId == StringID::INVALID)
        {
            return nullptr;
        }

        const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
        return objectsDb.getObject();
    }

    FORCE_INLINE bool isValid() const
    {
        if (objectId == StringID::INVALID)
        {
            return false;
        }
        const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
        if (Object *obj = objectsDb.getObject())
        {
            return INTERNAL_ObjectCoreAccessors::getAllocIdx(obj) == allocIdx;
        }
        return false;
    }
    FORCE_INLINE operator bool() const { return isValid(); }

    FORCE_INLINE void swap(WeakObjPtr<PtrType> &weakPtr)
    {
        std::swap(allocIdx, weakPtr.allocIdx);
        std::swap(objectId, weakPtr.objectId);
    }

    FORCE_INLINE void reset()
    {
        allocIdx = 0;
        objectId = StringID::INVALID;
    }

    // Detaches current ref counted resource without decrementing ref counter, Do not use it
    FORCE_INLINE void detachRef() { reset(); }
};
} // namespace cbe

template <typename Type>
struct std::hash<cbe::WeakObjPtr<Type>>
{
    NODISCARD size_t operator()(const cbe::WeakObjPtr<Type> &ptr) const noexcept
    {
        return HashUtility::hashAllReturn(ptr.objectId, ptr.allocIdx);
    }
};
