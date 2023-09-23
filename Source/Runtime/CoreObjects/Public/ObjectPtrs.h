/*!
 * \file ObjectPtrs.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
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
public:
    using ValueType = PtrType;
    using PointerType = ValueType *;

    using value_type = ValueType;
    using pointer_type = PointerType;

private:
    ObjectDbIdx dbIdx = CoreObjectsDB::InvalidDbIdx;
    PointerType objPtr = nullptr;

    friend std::hash<WeakObjPtr<PtrType>>;

public:
    WeakObjPtr() = default;

    WeakObjPtr(PointerType ptr) noexcept
    {
        if (cbe::isValid(ptr))
        {
            dbIdx = ptr->getDbIdx();
            objPtr = ptr;
        }
    }

    // Explicit copy and move constructor needed as compiler generates them without matching template
    // constructors
    WeakObjPtr(WeakObjPtr &&weakPtr) noexcept
        : dbIdx(weakPtr.dbIdx)
        , objPtr(weakPtr.objPtr)
    {
        weakPtr.detachRef();
    }
    WeakObjPtr(const WeakObjPtr &weakPtr) noexcept
        : dbIdx(weakPtr.dbIdx)
        , objPtr(weakPtr.objPtr)
    {}
    template <class InPtrType>
    WeakObjPtr(WeakObjPtr<InPtrType> &&weakPtr) noexcept
    {
        if (PointerType ptr = weakPtr.getAs<ValueType>())
        {
            this->operator= (ptr);
        }
        weakPtr.detachRef();
    }
    template <class InPtrType>
    WeakObjPtr(const WeakObjPtr<InPtrType> &weakPtr) noexcept
    {
        if (PointerType ptr = weakPtr.getAs<ValueType>())
        {
            this->operator= (ptr);
        }
    }

    ~WeakObjPtr() { reset(); }

    WeakObjPtr &operator= (PointerType ptr) noexcept
    {
        if (cbe::isValid(ptr))
        {
            dbIdx = ptr->getDbIdx();
            objPtr = ptr;
        }
        else
        {
            reset();
        }
        return *this;
    }

    // Explicit copy and move assignment needed as compiler generates them without matching template
    // assignments
    FORCE_INLINE WeakObjPtr &operator= (WeakObjPtr &&weakPtr) noexcept
    {
        if (this != &weakPtr)
        {
            dbIdx = weakPtr.dbIdx;
            objPtr = weakPtr.objPtr;
            weakPtr.detachRef();
        }
        return *this;
    }
    FORCE_INLINE WeakObjPtr &operator= (const WeakObjPtr &weakPtr) noexcept
    {
        dbIdx = weakPtr.dbIdx;
        objPtr = weakPtr.objPtr;
        return *this;
    }
    template <class InType>
    FORCE_INLINE WeakObjPtr &operator= (WeakObjPtr<InType> &&weakPtr) noexcept
    {
        if (this != &weakPtr)
        {
            if (PointerType ptr = weakPtr.getAs<ValueType>())
            {
                this->operator= (ptr);
                weakPtr.detachRef();
            }
        }
        return *this;
    }
    template <class InType>
    FORCE_INLINE WeakObjPtr &operator= (const WeakObjPtr<InType> &weakPtr) noexcept
    {
        if (PointerType ptr = weakPtr.getAs<ValueType>())
        {
            this->operator= (ptr);
        }
        return *this;
    }

    FORCE_INLINE PointerType operator->() const { return get(); }

    FORCE_INLINE bool operator!= (const WeakObjPtr &rhs) const { return !(*this == rhs); }
    template <typename Type>
    FORCE_INLINE bool operator!= (const WeakObjPtr<Type> &rhs) const
    {
        return !(*this == rhs);
    }
    FORCE_INLINE bool operator== (const WeakObjPtr &rhs) const { return dbIdx == rhs.dbIdx && objPtr == rhs.objPtr; }
    template <typename Type>
    FORCE_INLINE bool operator== (const WeakObjPtr<Type> &rhs) const
    {
        return dbIdx == rhs.dbIdx && objPtr == rhs.objPtr;
    }
    template <typename Type>
    FORCE_INLINE bool operator== (Type *rhs) const
    {
        return get() == rhs;
    }

    FORCE_INLINE bool operator< (const WeakObjPtr &rhs) const { return dbIdx == rhs.dbIdx ? objPtr < rhs.objPtr : dbIdx < rhs.dbIdx; }
    template <typename Type>
    FORCE_INLINE bool operator< (const WeakObjPtr<Type> &rhs) const
    {
        return dbIdx == rhs.dbIdx ? objPtr < rhs.objPtr : dbIdx < rhs.dbIdx;
    }

    template <typename AsType>
    FORCE_INLINE AsType *getAs() const
    {
        return cast<AsType>(get());
    }
    // For compliance with SharedPtr
    FORCE_INLINE PointerType get() const
    {
        if (!isValid())
        {
            return nullptr;
        }
        return static_cast<PointerType>(objPtr);
    }

    // Checks if set objectId is valid now
    FORCE_INLINE bool isValid() const
    {
        if (!isSet())
        {
            return false;
        }
        const CoreObjectsDB &objectsDb = ICoreObjectsModule::objectsDB();
        Object *obj = objectsDb.getObject(dbIdx);
        return obj == objPtr && cbe::isValidAlloc(objPtr);
    }
    FORCE_INLINE explicit operator bool () const { return isValid(); }

    // Check if this WeakPtr is set
    FORCE_INLINE bool isSet() const { return dbIdx != CoreObjectsDB::InvalidDbIdx && objPtr != nullptr; }

    FORCE_INLINE void swap(WeakObjPtr<ValueType> &weakPtr)
    {
        std::swap(dbIdx, weakPtr.dbIdx);
        std::swap(objPtr, weakPtr.objPtr);
    }

    FORCE_INLINE void reset()
    {
        dbIdx = CoreObjectsDB::InvalidDbIdx;
        objPtr = nullptr;
    }

    // Detaches current ref counted resource without decrementing ref counter, Do not use it
    FORCE_INLINE void detachRef() { reset(); }
};

using WeakObjectPtr = WeakObjPtr<Object>;

struct COREOBJECTS_EXPORT ObjectPath
{
public:
    ObjectDbIdx dbIdx = CoreObjectsDB::InvalidDbIdx;
    String packagePath;
    String outerPath;
    String objectName;

    ObjectPath() = default;
    ObjectPath(const ObjectPath &) = default;
    ObjectPath(ObjectPath &&) = default;
    ObjectPath &operator= (const ObjectPath &) = default;
    ObjectPath &operator= (ObjectPath &&) = default;

    explicit ObjectPath(const TChar *fullPath) noexcept { (*this) = fullPath; }
    explicit ObjectPath(Object *obj) noexcept { (*this) = obj; }
    ObjectPath &operator= (const TChar *fullPath) noexcept;
    ObjectPath &operator= (Object *obj) noexcept;

    ObjectPath(Object *outerObj, const TChar *objectName) noexcept;

    FORCE_INLINE bool operator!= (const ObjectPath &rhs) const { return !(*this == rhs); }
    FORCE_INLINE bool operator== (const ObjectPath &rhs) const { return getFullPath() == rhs.getFullPath(); }
    template <typename Type>
    FORCE_INLINE bool operator== (Type *rhs) const
    {
        return getObject() == rhs;
    }
    FORCE_INLINE bool operator< (const ObjectPath &rhs) const { return getFullPath() < rhs.getFullPath(); }

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
    bool isValid() const;
    FORCE_INLINE explicit operator bool () const { return isValid(); }
    FORCE_INLINE void reset()
    {
        dbIdx = CoreObjectsDB::InvalidDbIdx;
        packagePath = outerPath = objectName = TCHAR("");
    }

    // Tries to refresh the cached dbIdx and updates it if it is outdated
    void refreshCache();
    bool isValidCache() const;
};

} // namespace cbe

template <typename Type>
struct std::hash<cbe::WeakObjPtr<Type>>
{
    NODISCARD size_t operator() (const cbe::WeakObjPtr<Type> &ptr) const noexcept { return HashUtility::hashAllReturn(ptr.dbIdx, ptr.objPtr); }
};

template <>
struct std::hash<cbe::ObjectPath>
{
    NODISCARD size_t operator() (const cbe::ObjectPath &ptr) const noexcept { return HashUtility::hashAllReturn(ptr.getFullPath()); }
};
