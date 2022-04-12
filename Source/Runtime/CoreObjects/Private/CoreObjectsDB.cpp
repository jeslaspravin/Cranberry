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
#include "CoreObjectAllocator.h"

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

CoreObjectsDB::NodeIdxType CoreObjectsDB::addRootObject(StringID objectId, const ObjectData& objectData)
{
    debugAssert(objectId.isValid() && !hasObject(objectId));

    NodeIdxType nodeIdx = objectTree.add(objectData);
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

CBE::Object* CoreObjectsDB::getObject(NodeIdxType nodeidx) const
{
    if (objectTree.isValid(nodeidx))
    {
        const ObjectData& objData = objectTree[nodeidx];
        return CBE::getObjAllocator(objData.clazz)->getAt<CBE::Object>(objData.allocIdx);
    }
    return nullptr;
}

void CoreObjectsDB::getSubobjects(std::vector<CBE::Object*>& subobjs, NodeIdxType nodeidx) const
{
    std::vector<NodeIdxType> subObjIndices;
    objectTree.getChildren(subObjIndices, nodeidx, true);
    for (NodeIdxType subnodeIdx : subObjIndices)
    {
        if (CBE::Object* obj = getObject(subnodeIdx))
        {
            subobjs.emplace_back(obj);
        }
    }
}
