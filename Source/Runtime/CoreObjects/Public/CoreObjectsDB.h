/*!
 * \file CoreObjectsDB.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObjectTypes.h"
#include "CoreObjectsExports.h"
#include "String/StringID.h"
#include "Types/Containers/FlatTree.h"

/**
 * CoreObjectsDB
 *
 * Contains objects hierarchy data and object SID, Alloc idx in database separate from class for quick
 * access Possible use is for garbage collector in future
 */
class COREOBJECTS_EXPORT CoreObjectsDB
{
public:
    using NodeIdxType = SizeT;
    struct ObjectData
    {
        // Below 2 can be used to retrieve object from allocator directly
        CBEClass clazz;
        ObjectAllocIdx allocIdx;
        StringID sid;
    };

    std::unordered_map<StringID, NodeIdxType> objectIdToNodeIdx;
    FlatTree<ObjectData, NodeIdxType> objectTree;

public:
    // Removes object and all its sub-object from db
    void removeObject(StringID objectId);
    NodeIdxType addObject(StringID objectId, const ObjectData &objectData, StringID parent);
    NodeIdxType addRootObject(StringID objectId, const ObjectData &objectData);
    void setObject(StringID currentId, StringID newId);
    // Invalid newParent clears current parent
    void setObjectParent(StringID objectId, StringID newParent);

    FORCE_INLINE bool hasObject(StringID objectId) const
    {
        auto itr = objectIdToNodeIdx.find(objectId);
        if (itr != objectIdToNodeIdx.cend())
        {
            debugAssert(objectTree.isValid(itr->second));
            return objectTree.isValid(itr->second);
        }
        return false;
    }
    FORCE_INLINE bool hasObject(NodeIdxType nodeIdx) const
    {
        if (objectTree.isValid(nodeIdx))
        {
            auto itr = objectIdToNodeIdx.find(objectTree[nodeIdx].sid);
            debugAssert(itr != objectIdToNodeIdx.cend());
            return itr != objectIdToNodeIdx.cend();
        }
        return false;
    }

    CBE::Object *getObject(NodeIdxType nodeidx) const;
    FORCE_INLINE CBE::Object *getObject(StringID objectId) const
    {
        auto itr = objectIdToNodeIdx.find(objectId);
        if (itr != objectIdToNodeIdx.cend())
        {
            return getObject(itr->second);
        }
        return nullptr;
    }

    void getSubobjects(std::vector<CBE::Object *> &subobjs, NodeIdxType nodeidx) const;
    FORCE_INLINE void getSubobjects(std::vector<CBE::Object *> &subobjs, StringID objectId) const
    {
        auto itr = objectIdToNodeIdx.find(objectId);
        if (itr != objectIdToNodeIdx.cend())
        {
            getSubobjects(subobjs, itr->second);
        }
    }
};