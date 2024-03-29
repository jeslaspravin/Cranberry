/*!
 * \file CoreObjectAllocator.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CoreObjectAllocator.h"
#include "Property/Property.h"

std::unordered_map<CBEClass, cbe::ObjectAllocatorBase *> *gCBEObjectAllocators = nullptr;

namespace cbe
{
std::unordered_map<CBEClass, ObjectAllocatorBase *> &createGObjectAllocators()
{
    static std::unordered_map<CBEClass, ObjectAllocatorBase *> singletonObjectAllocatorsMap;
    return singletonObjectAllocatorsMap;
}

void initializeObjectAllocators()
{
    if (gCBEObjectAllocators == nullptr)
    {
        gCBEObjectAllocators = &createGObjectAllocators();
    }
}

ObjectAllocatorBase *getObjAllocator(CBEClass clazz)
{
    if (!gCBEObjectAllocators)
    {
        initializeObjectAllocators();
    }
    auto itr = gCBEObjectAllocators->find(clazz);
    return itr != gCBEObjectAllocators->end() ? itr->second : nullptr;
}

} // namespace cbe
