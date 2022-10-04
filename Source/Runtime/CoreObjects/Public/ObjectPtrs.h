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
    ObjectAllocIdx allocIdx = 0;
    NameString objectPath;

    friend std::hash<WeakObjPtr<PtrType>>;

public:
    WeakObjPtr() = default;

    WeakObjPtr(PtrType *ptr)
    {
        if (cbe::isValid(ptr))
        {
            allocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(ptr);
            objectPath = ptr->getFullPath().getChar();
        }
    }

    // Explicit copy and move constructor needed as compiler generates them without matching template
    // constructors
    WeakObjPtr(WeakObjPtr &&weakPtr)
        : allocIdx(weakPtr.allocIdx)
        , objectPath(weakPtr.objectPath)
    {
        weakPtr.detachRef();
    }
    WeakObjPtr(const WeakObjPtr &weakPtr)
        : allocIdx(weakPtr.allocIdx)
        , objectPath(weakPtr.objectPath)
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
            objectPath = ptr->getFullPath().getChar();
        }
        else
        {
            reset();
        }
        return *this;
    }

    // Explicit copy and move assignment needed as compiler generates them without matching template
    // assignments
    FORCE_INLINE WeakObjPtr &operator=(WeakObjPtr &&weakPtr)
    {
        if (this != &weakPtr)
        {
            allocIdx = weakPtr.allocIdx;
            objectPath = weakPtr.objectPath;
            weakPtr.detachRef();
        }
        return *this;
    }
    FORCE_INLINE WeakObjPtr &operator=(const WeakObjPtr &weakPtr)
    {
        allocIdx = weakPtr.allocIdx;
        objectPath = weakPtr.objectPath;
        return *this;
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

    FORCE_INLINE bool operator!=(const WeakObjPtr &rhs) const { return !(*this == rhs); }
    template <typename Type>
    FORCE_INLINE bool operator!=(const WeakObjPtr<Type> &rhs) const
    {
        return !(*this == rhs);
    }
    FORCE_INLINE bool operator==(const WeakObjPtr &rhs) const
    {
        return allocIdx == rhs.allocIdx && objectPath == rhs.objectPath && objectPath.toString().isEqual(rhs.objectPath.toString());
    }
    template <typename Type>
    FORCE_INLINE bool operator==(const WeakObjPtr<Type> &rhs) const
    {
        return allocIdx == rhs.allocIdx && objectPath == rhs.objectPath && objectPath.toString().isEqual(rhs.objectPath.toString());
    }
    template <typename Type>
    FORCE_INLINE bool operator==(Type *rhs) const
    {
        return get() == rhs;
    }

    FORCE_INLINE bool operator<(const WeakObjPtr &rhs) const
    {
        return objectPath == rhs.objectPath ? allocIdx < rhs.allocIdx : objectPath < rhs.objectPath;
    }
    template <typename Type>
    FORCE_INLINE bool operator<(const WeakObjPtr<Type> &rhs) const
    {
        return objectPath == rhs.objectPath ? allocIdx < rhs.allocIdx : objectPath < rhs.objectPath;
    }

    template <typename AsType>
    FORCE_INLINE AsType *getAs() const
    {
        return cast<AsType>(get());
    }
    // For compliance with SharedPtr
    FORCE_INLINE PtrType *get() const
    {
        if (!objectPath.isValid())
        {
            return nullptr;
        }
        return static_cast<PtrType *>(cbe::get(StringID(objectPath), objectPath.toString().getChar()));
    }

    // Checks if set objectId is valid now
    FORCE_INLINE bool isValid() const
    {
        if (!objectPath.isValid())
        {
            return false;
        }
        if (Object *obj = cbe::get(StringID(objectPath), objectPath.toString().getChar()))
        {
            return INTERNAL_ObjectCoreAccessors::getAllocIdx(obj) == allocIdx;
        }
        return false;
    }
    FORCE_INLINE explicit operator bool() const { return isValid(); }

    // Check if this WeakPtr is set
    FORCE_INLINE bool isSet() const { return objectPath.isValid(); }

    FORCE_INLINE void swap(WeakObjPtr<PtrType> &weakPtr)
    {
        std::swap(allocIdx, weakPtr.allocIdx);
        std::swap(objectPath, weakPtr.objectPath);
    }

    FORCE_INLINE void reset()
    {
        allocIdx = 0;
        objectPath = NameString();
    }

    // Detaches current ref counted resource without decrementing ref counter, Do not use it
    FORCE_INLINE void detachRef() { reset(); }
};

struct COREOBJECTS_EXPORT ObjectPath
{
public:
    ObjectAllocIdx allocIdx = 0;
    String packagePath;
    String outerPath;
    String objectName;

    ObjectPath() = default;
    ObjectPath(const ObjectPath &) = default;
    ObjectPath(ObjectPath &&) = default;
    ObjectPath &operator=(const ObjectPath &) = default;
    ObjectPath &operator=(ObjectPath &&) = default;

    explicit ObjectPath(const TChar *fullPath) { (*this) = fullPath; }
    explicit ObjectPath(Object *obj) { (*this) = obj; }
    ObjectPath &operator=(const TChar *fullPath);
    ObjectPath &operator=(Object *obj);

    ObjectPath(Object *outerObj, const TChar *objectName);

    FORCE_INLINE bool operator!=(const ObjectPath &rhs) const { return !(*this == rhs); }
    FORCE_INLINE bool operator==(const ObjectPath &rhs) const { return allocIdx == rhs.allocIdx && getFullPath() == rhs.getFullPath(); }
    template <typename Type>
    FORCE_INLINE bool operator==(Type *rhs) const
    {
        return getObject() == rhs;
    }
    FORCE_INLINE bool operator<(const ObjectPath &rhs) const
    {
        return allocIdx == rhs.allocIdx ? getFullPath() < rhs.getFullPath() : allocIdx < rhs.allocIdx;
    }

    const String &getPackagePath() const { return packagePath; }
    const String &getOuterPath() const { return outerPath; }
    const String &getObjectName() const { return objectName; }
    String getFullPath() const;

    // Return nullptr if no object found, Tries to load which could be slow
    Object *getObject() const;
    template <ObjectType AsType>
    AsType *getObject() const
    {
        return cast<AsType>(getObject());
    }

    // Checks if set objectId is valid now, Without loading
    FORCE_INLINE bool isValid() const
    {
        Object *obj = get(getFullPath().getChar());
        if (cbe::isValid(obj))
        {
            return INTERNAL_ObjectCoreAccessors::getAllocIdx(obj) == allocIdx;
        }
        return false;
    }
    FORCE_INLINE explicit operator bool() const { return isValid(); }
    FORCE_INLINE void reset()
    {
        allocIdx = 0;
        packagePath = outerPath = objectName = TCHAR("");
    }
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

template <>
struct std::hash<cbe::ObjectPath>
{
    NODISCARD size_t operator()(const cbe::ObjectPath &ptr) const noexcept
    {
        return HashUtility::hashAllReturn(ptr.allocIdx, ptr.getFullPath());
    }
};
