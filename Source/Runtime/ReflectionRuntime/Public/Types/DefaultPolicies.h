/*!
 * \file DefaultPolicies.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
/**
 * Default heap allocated construction policies for class and structs
 * If you want to override how a reflected object is constructed and allocated create a new policy with
 * same signature as below and give your implementations Then typedef it to typename
 * HeapConstructionPolicy inside the class in public/protected visibility or simple use
 * OVERRIDE_CONSTRUCTION_POLICY macro after GENERATED_CODES() macro with policy passed in
 *
 * Always pass in base type as the destructing ptr, As other derived types are somewhere after base type
 * and destruction will not be called properly Above note is mandatory if there is diamond inheritance in
 * your hierarchy(Not recommended to have such hierarchy)
 *
 * allocate and deallocate functions are used to separate allocation from construction
 * newObject and deleteObject are used as alloc + construction and delete. In this case canDeallocate
 * must always return false
 *
 * CtorPolicyHelper - Functions will be used by reflected code
 */
class DefaultConstructionPolicy
{
public:
    // Called for raw allocation deallocation has to be handled by yourself
    template <typename Type>
    static void *allocate()
    {
        return nullptr;
    }
    // Return true if deallocate is valid for this policy and deallocation is allowed
    template <typename Type>
    static bool canDeallocate(void *ptr)
    {
        return false;
    }
    template <typename Type>
    static void deallocate(void *ptr)
    {}

    // Called for new Type(...) allocation if raw allocation failed
    template <typename Type, typename... CtorArgs>
    static Type *newObject(CtorArgs &&...args)
    {
        return new Type(std::forward<CtorArgs>(args)...);
    }
    // Called if deallocate is not available
    template <typename Type>
    static void deleteObject(Type *ptr)
    {
        delete ptr;
    }

    // Policy available function used only in case of above raw allocation being successful
    // Must call the constructor in this function for your custom policy
    template <typename Type, typename... CtorArgs>
    static Type *construct(void *allocatedPtr, CtorArgs &&...args)
    {
        return nullptr;
    }
    // Must call the destructor in this function for your custom policy
    template <typename Type>
    static void destruct(void *ptr)
    {}
};

namespace CtorPolicyHelper
{
template <typename CtorPolicy, typename Type>
FORCE_INLINE static void *allocateObject()
{
    return CtorPolicy::template allocate<Type>();
}

// If incoming pointer is null, Then this function uses newObject from construction policy to create and
// construct object
template <typename CtorPolicy, typename Type, typename... CtorArgs>
FORCE_INLINE static Type *constructObject(void *allocatedPtr, CtorArgs &&...args)
{
    // if (void* allocatedPtr = CtorPolicy::template allocate<Type>())
    if (allocatedPtr)
    {
        return CtorPolicy::template construct<Type>(allocatedPtr, std::forward<CtorArgs>(args)...);
    }
    Type *objPtr = CtorPolicy::template newObject<Type>(std::forward<CtorArgs>(args)...);
    fatalAssert(objPtr,
        "if passed in ptr is null newObject of construction policy must create and construct object");
    return objPtr;
}

// Always pass in base type as the destructing ptr, As other derived types are somewhere after base type
// and destruction will not be called properly
template <typename CtorPolicy, typename Type>
FORCE_INLINE static void destructObject(void *ptr)
{
    if (CtorPolicy::template canDeallocate<Type>(ptr))
    {
        CtorPolicy::template destruct<Type>(ptr);
        CtorPolicy::template deallocate<Type>(ptr);
        return;
    }

    CtorPolicy::template deleteObject<Type>((Type *)ptr);
}
} // namespace CtorPolicyHelper