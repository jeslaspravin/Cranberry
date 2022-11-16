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

#include "CoreObjectsExports.h"
#include "CoreObjectAllocator.h"
#include "ReflectionMacros.h"
#include "Serialization/ObjectArchive.h"
#include "Serialization/CommonTypesSerialization.h"
#include "Types/CompilerDefines.h"

// Needed only while parsing to allow all CoreObject users to have this
#ifdef __REF_PARSE__
#include "ReflectNativeStructs.h"
#endif

#include "CBEObject.gen.h"

class CBEObjectConstructionPolicy
{
public:
    // Called for raw allocation deallocation has to be handled by yourself
    template <typename Type>
    static void *allocate();
    template <typename Type>
    static bool canDeallocate(void */*ptr*/)
    {
        return true;
    }
    template <typename Type>
    static void deallocate(void *ptr);

    // Must call the constructor in this function for your custom policy
    template <typename Type, typename... CtorArgs>
    static Type *construct(void *allocatedPtr, CtorArgs &&...args);
    // Must call the destructor in this function for your custom policy
    template <typename Type>
    static void destruct(void *ptr);

    // Unwanted impls
    // Called for new Type(...) allocation if raw allocation failed
    template <typename Type, typename... CtorArgs>
    static Type *newObject(CtorArgs &&.../*args*/)
    {
        fatalAssertf(false, "newObject is not supported interface and must not happen");
        return nullptr;
    }
    template <typename Type>
    static void deleteObject(Type *)
    {
        fatalAssertf(false, "deleteObject is not supported interface and must not happen");
    }
};

COMPILER_PRAGMA(COMPILER_PUSH_WARNING)
COMPILER_PRAGMA(COMPILER_DISABLE_WARNING(WARN_UNINITIALIZED))

namespace cbe
{
// Just a class to avoid overwriting base properties when constructing
// Example string will always be constructed to empty which is not acceptable behavior for us
class ObjectBase
{
protected:
    String objectName;

protected:
    ObjectBase() = delete;
    MAKE_TYPE_NONCOPY_NONMOVE(ObjectBase)

    ObjectBase(String inObjectName)
        : objectName(inObjectName)
    {}
};

class COREOBJECTS_EXPORT Object : private ObjectBase
{
    GENERATED_CODES();

public:
    OVERRIDE_CONSTRUCTION_POLICY(CBEObjectConstructionPolicy);

private:
    friend INTERNAL_ObjectCoreAccessors;
    friend CBEObjectConstructionPolicy;

    Object *objOuter;
    EObjectFlags flags;
    StringID sid;
    ObjectAllocIdx allocIdx;

protected:
    void markReadyForDestroy() { SET_BITS(flags, EObjectFlagBits::ObjFlag_MarkedForDelete); }

public:
    Object()
        : ObjectBase(objectName)
        , objOuter(objOuter)
        , flags(flags)
        , sid(sid)
        , allocIdx(allocIdx)
    {}

    virtual ~Object() = default;

    void destroyObject();
    void beginDestroy();
    /**
     * This gets called after complete construction from either Class or something else(Example after load or after construction from template)
     * This does not mean it will not get called before that. It has to be handled after checking object flags
     */
    void constructed()
    {
        // Also change cbe::create()
        debugAssertf(
            NO_BITS_SET(flags, EObjectFlagBits::ObjFlag_PackageLoadPending),
            "constructed called before load is finished! Try using INTERNAL_create"
        );
        onConstructed();
    }
    // Will get called after loaded from package
    void postLoad() { onPostLoad(); }
    // Will get called after serialize and linking is done
    void postSerialize(const ObjectArchive &ar) { onPostSerialize(ar); }

    FORCE_INLINE Object *getOuter() const { return objOuter; }
    FORCE_INLINE Object *getOuterMost() const
    {
        Object *outer = this->getOuter();
        while (outer && outer->getOuter())
        {
            outer = outer->getOuter();
        }
        return outer;
    }
    template <typename Type>
    FORCE_INLINE Type *getOuterOfType() const
    {
        return static_cast<Type *>(getOuterOfType(Type::staticType()));
    }
    FORCE_INLINE Object *getOuterOfType(CBEClass clazz) const
    {
        Object *outer = this->getOuter();
        while (outer && outer->getType() != clazz)
        {
            outer = outer->getOuter();
        }
        return outer;
    }
    bool hasOuter(Object *checkOuter) const { return getOuter() && (getOuter() == checkOuter || getOuter()->hasOuter(checkOuter)); }

    FORCE_INLINE EObjectFlags getFlags() const { return flags; }
    FORCE_INLINE EObjectFlags collectAllFlags() const
    {
        EObjectFlags retVal = flags;
        Object *outer = this->getOuter();
        while (outer)
        {
            retVal |= outer->flags;
            outer = outer->getOuter();
        }
        return retVal;
    }

    FORCE_INLINE const String &getName() const { return objectName; }
    FORCE_INLINE StringID getStringID() const { return sid; }
    String getFullPath() const;

    virtual void destroy() {}
    virtual void onConstructed() {}
    virtual void onPostLoad() {}
    virtual void onPostSerialize(const ObjectArchive &/*ar*/) {}
    virtual ObjectArchive &serialize(ObjectArchive &ar) { return ar; }
} META_ANNOTATE(BaseType; NoExport);
} // namespace cbe

COMPILER_PRAGMA(COMPILER_POP_WARNING)

template <typename Type>
void *CBEObjectConstructionPolicy::allocate()
{
    ObjectAllocIdx allocIdx;
    Type *ptr = (Type *)cbe::getObjAllocator<Type>().allocate(allocIdx);
    CBEMemory::memZero(ptr, sizeof(Type));

    cbe::Object *objPtr = static_cast<cbe::Object *>(ptr);
    objPtr->allocIdx = allocIdx;
    return ptr;
}
template <typename Type>
void CBEObjectConstructionPolicy::deallocate(void *ptr)
{
    cbe::Object *objPtr = static_cast<cbe::Object *>(ptr);
    cbe::getObjAllocator<Type>().free(ptr, objPtr->allocIdx);
}

template <typename Type, typename... CtorArgs>
Type *CBEObjectConstructionPolicy::construct(void *allocatedPtr, CtorArgs &&...args)
{
    return new (allocatedPtr) Type(std::forward<CtorArgs>(args)...);
}
template <typename Type>
void CBEObjectConstructionPolicy::destruct(void *ptr)
{
    cbe::Object *objPtr = static_cast<cbe::Object *>(ptr);
    static_cast<Type *>(objPtr)->~Type();
}
