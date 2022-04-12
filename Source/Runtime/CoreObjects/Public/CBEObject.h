/*!
 * \file CBEObject.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CompilerDefines.h"
#include "CoreObjectsExports.h"
#include "ReflectionMacros.h"
#include "CoreObjectAllocator.h"

#include "CBEObject.gen.h"

class CBEObjectConstructionPolicy
{
public:
    // Called for raw allocation deallocation has to be handled by yourself
    template <typename Type>
    static void* allocate();
    template <typename Type>
    static bool canDeallocate(void* ptr) { return true; }
    template <typename Type>
    static void deallocate(void* ptr);

    // Must call the constructor in this function for your custom policy
    template <typename Type, typename... CtorArgs>
    static Type* construct(void* allocatedPtr, CtorArgs&&... args);
    // Must call the destructor in this function for your custom policy
    template <typename Type>
    static void destruct(void* ptr);

    // Unwanted impls
    // Called for new Type(...) allocation if raw allocation failed
    template <typename Type, typename... CtorArgs>
    static Type* newObject(CtorArgs&&... args) { fatalAssert(false, "newObject is not supported interface and must not happen"); return nullptr; }
    template <typename Type>
    static void deleteObject(Type* ptr) { fatalAssert(false, "deleteObject is not supported interface and must not happen"); }
};

COMPILER_PRAGMA(COMPILER_PUSH_WARNING)
COMPILER_PRAGMA(COMPILER_DISABLE_WARNING(WARN_UNINITIALIZED))

namespace CBE
{
    // Just a class to avoid overwriting base properties when constructing
    // Example string will always be constructed to empty which is not acceptable behavior for us
    class ObjectBase
    {
    protected:
        String objectName;
    protected:
        ObjectBase() = delete;
        ObjectBase(ObjectBase&&) = delete;
        ObjectBase(const ObjectBase&) = delete;

        ObjectBase(String inObjectName)
            : objectName(inObjectName)
        {}
    };

    class META_ANNOTATE_API(COREOBJECTS_EXPORT, BaseType) Object : private ObjectBase
    {
        GENERATED_CODES();
    public:
        OVERRIDE_CONSTRUCTION_POLICY(CBEObjectConstructionPolicy);
    private:
        friend PrivateObjectCoreAccessors;
        friend CBEObjectConstructionPolicy;

        Object* objOuter;
        EObjectFlags flags;
        StringID sid;
        ObjectAllocIdx allocIdx;
    public:
        Object()
            : ObjectBase(objectName)
            , objOuter(objOuter)
            , flags(flags)
            , sid(sid)
            , allocIdx(allocIdx)
        {}

        virtual ~Object();

        FORCE_INLINE Object* getOuter() const { return objOuter; }
        FORCE_INLINE EObjectFlags getFlags() const { return flags; }
        FORCE_INLINE const String& getName() const { return objectName; }
        FORCE_INLINE StringID getStringID() const { return sid; }
        String getFullPath() const;
    };
}

COMPILER_PRAGMA(COMPILER_POP_WARNING)


template <typename Type>
void* CBEObjectConstructionPolicy::allocate()
{
    ObjectAllocIdx allocIdx;
    Type* ptr = (Type*)CBE::getObjAllocator<Type>().allocate(allocIdx);
    CBEMemory::memZero(ptr, sizeof(Type));

    CBE::Object* objPtr = static_cast<CBE::Object*>(ptr);
    objPtr->allocIdx = allocIdx;
    return ptr;
}
template <typename Type>
void CBEObjectConstructionPolicy::deallocate(void* ptr)
{
    CBE::Object* objPtr = static_cast<CBE::Object*>(ptr);
    CBE::getObjAllocator<Type>().free(ptr, objPtr->allocIdx);
}

template <typename Type, typename... CtorArgs>
Type* CBEObjectConstructionPolicy::construct(void* allocatedPtr, CtorArgs&&... args)
{
    return new (allocatedPtr)Type(std::forward<CtorArgs>(args)...);
}
template <typename Type>
void CBEObjectConstructionPolicy::destruct(void* ptr)
{
    CBE::Object* objPtr = static_cast<CBE::Object*>(ptr);
    static_cast<Type*>(objPtr)->~Type();
}
