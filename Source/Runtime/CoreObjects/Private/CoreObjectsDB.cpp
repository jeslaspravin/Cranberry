/*!
 * \file CoreObjectsDB.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CoreObjectsDB.h"

void CoreObjectsDB::removeObject(StringID objectId)
{
    debugAssert(objectId.isValid() && hasObject(objectId));
    auto objItr = objectIdToNodeIdx.find(objectId);
    objectTree.remove(objItr->second);
    objectIdToNodeIdx.erase(objItr);
}

CoreObjectsDB::NodeIdxType CoreObjectsDB::addObject(StringID objectId, const ObjectData& objectData, StringID parent)
{
    debugAssert(objectId.isValid() && !hasObject(objectId) && hasObject(parent));

    auto parentItr = objectIdToNodeIdx.find(parent);
    NodeIdxType nodeIdx = objectTree.add(objectData, parentItr->second);
    objectIdToNodeIdx[objectId] = nodeIdx;
    return nodeIdx;
}

void CoreObjectsDB::setObject(StringID currentId, StringID newId)
{
    debugAssert(currentId.isValid() && newId.isValid() && currentId != newId
        && !hasObject(newId) && hasObject(currentId));

    auto objItr = objectIdToNodeIdx.find(currentId);
    NodeIdxType nodeIdx = objItr->second;
    objectIdToNodeIdx.erase(objItr);
    objectIdToNodeIdx[newId] = nodeIdx;
    objectTree[nodeIdx].sid = newId;
}

void CoreObjectsDB::setObjectParent(StringID objectId, StringID newParent)
{
    debugAssert(objectId.isValid() && hasObject(objectId));

    auto objItr = objectIdToNodeIdx.find(objectId);
    if (newParent.isValid())
    {
        debugAssert(hasObject(newParent));
        auto parentItr = objectIdToNodeIdx.find(newParent);
        objectTree.relinkTo(objItr->second, parentItr->second);
    }
    else
    {
        objectTree.relinkTo(objItr->second);
    }
}
