/*!
 * \file CoreObjectAllocator.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CoreObjectAllocator.h"

std::unordered_map<const ClassProperty*, CBE::ObjectAllocatorBase*>* gCBEObjectAllocators = nullptr;

namespace CBE
{
    std::unordered_map<const ClassProperty*, ObjectAllocatorBase*>& createGObjectAllocators()
    {
        static std::unordered_map<const ClassProperty*, ObjectAllocatorBase*> singletonObjectAllocatorsMap;
        return singletonObjectAllocatorsMap;
    }

    void initializeObjectAllocators()
    {
        if (gCBEObjectAllocators == nullptr)
        {
            gCBEObjectAllocators = &createGObjectAllocators();
        }
    }

    ObjectAllocatorBase* getObjAllocator(const ClassProperty* classType)
    {
        if (!gCBEObjectAllocators)
        {
            initializeObjectAllocators();
        }
        auto itr = gCBEObjectAllocators->find(classType);
        return itr != gCBEObjectAllocators->end() ? itr->second : nullptr;
    }

}
