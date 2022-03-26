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

// Default heap allocated construction policies for class and structs
// If you want to override how a reflected object is constructed and allocated create a new policy with same signature as below and give your implementations
// Then typedef it to typename HeapConstructionPolicy inside the class in public/protected visibility 
// or simple use OVERRIDE_CONSTRUCTION_POLICY macro after GENERATED_CODES() macro with policy passed in
class DefaultConstructionPolicy
{
public:
    // Called for raw allocation deallocation has to be handled by yourself
    template <typename Type>
    static void* allocate() { return nullptr; }
    // Called for new Type(...) allocation if raw allocation failed
    template <typename Type, typename... CtorArgs>
    static Type* newObject(CtorArgs&&... args)
    {
        return new Type(std::forward<CtorArgs>(args)...);
    }

    // Policy available function used only in case of above raw allocation being successful
    template <typename Type, typename... CtorArgs>
    static void preConstruct(void* allocatedPtr, CtorArgs&&... args) {}
    // Must call the constructor in this function for your custom policy
    template <typename Type, typename... CtorArgs>
    static Type* construct(void* allocatedPtr, CtorArgs&&... args) { return nullptr; }
    template <typename Type, typename... CtorArgs>
    static void postConstruct(Type* object, CtorArgs&&... args) {}
};

namespace CtorPolicyHelper
{
    template <typename CtorPolicy, typename Type, typename... CtorArgs>
    static Type* constructObject(CtorArgs&&... args)
    {
        if (void* allocatedPtr = CtorPolicy::template allocate<Type>())
        {
            CtorPolicy::template preConstruct<Type>(allocatedPtr, std::forward<CtorArgs>(args)...);
            Type* objectPtr = CtorPolicy::template construct<Type>(allocatedPtr, std::forward<CtorArgs>(args)...);
            CtorPolicy::template postConstruct(objectPtr, std::forward<CtorArgs>(args)...);
            return objectPtr;
        }

        return CtorPolicy::template newObject<Type>(std::forward<CtorArgs>(args)...);
    }
}