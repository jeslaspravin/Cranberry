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
#include "Types/Platform/Threading/CoPaT/JobSystem.h"

#include <shared_mutex>

FORCE_INLINE bool CoreObjectsDB::isMainThread() const
{
    return copat::JobSystem::get()->getCurrentThreadType() == copat::EJobThreadType::MainThread;
}

void CoreObjectsDB::SharedLockObjectsDB::lockShared() const { objsDb->dbLock->lock_shared(); }
void CoreObjectsDB::SharedLockObjectsDB::unlockShared() const { objsDb->dbLock->unlock_shared(); }

CoreObjectsDB::CoreObjectsDB() { dbLock = new SharedLockType; }

CoreObjectsDB::~CoreObjectsDB()
{
    delete dbLock;
    dbLock = nullptr;
}

void CoreObjectsDB::removeObject(StringID objectId)
{
    fatalAssertf(isMainThread(), "Remove object %s must be done from main thread!", objectId);
    debugAssert(objectId.isValid() && hasObject(objectId));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    auto objItr = objectIdToNodeIdx.find(objectId);
    objectTree.remove(objItr->second);
    objectIdToNodeIdx.erase(objItr);
}

CoreObjectsDB::NodeIdxType CoreObjectsDB::addObject(StringID objectId, const ObjectData &objectData, StringID parent)
{
    fatalAssertf(isMainThread(), "Add object %s must be done from main thread!", objectId);
    debugAssert(objectId.isValid() && !hasObject(objectId) && hasObject(parent));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    auto parentItr = objectIdToNodeIdx.find(parent);
    NodeIdxType nodeIdx = objectTree.add(objectData, parentItr->second);
    objectIdToNodeIdx[objectId] = nodeIdx;
    return nodeIdx;
}

CoreObjectsDB::NodeIdxType CoreObjectsDB::addRootObject(StringID objectId, const ObjectData &objectData)
{
    fatalAssertf(isMainThread(), "Add object %s must be done from main thread!", objectId);
    debugAssert(objectId.isValid() && !hasObject(objectId));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    NodeIdxType nodeIdx = objectTree.add(objectData);
    objectIdToNodeIdx[objectId] = nodeIdx;
    return nodeIdx;
}

void CoreObjectsDB::setObject(StringID currentId, StringID newId)
{
    fatalAssertf(isMainThread(), "Set object %s must be done from main thread!", currentId);
    debugAssert(currentId.isValid() && newId.isValid() && currentId != newId && !hasObject(newId) && hasObject(currentId));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    auto objItr = objectIdToNodeIdx.find(currentId);
    NodeIdxType nodeIdx = objItr->second;
    objectIdToNodeIdx.erase(objItr);
    objectIdToNodeIdx[newId] = nodeIdx;
    objectTree[nodeIdx].sid = newId;
}

void CoreObjectsDB::setObjectParent(StringID objectId, StringID newParent)
{
    fatalAssertf(isMainThread(), "Set parent object %s must be done from main thread!", objectId);
    debugAssert(objectId.isValid() && hasObject(objectId));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

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

cbe::Object *CoreObjectsDB::getObject(NodeIdxType nodeidx) const
{
    SharedLockObjectsDB scopedLock(this);

    if (objectTree.isValid(nodeidx))
    {
        const ObjectData &objData = objectTree[nodeidx];
        return cbe::getObjAllocator(objData.clazz)->getAt<cbe::Object>(objData.allocIdx);
    }
    return nullptr;
}

void CoreObjectsDB::getSubobjects(std::vector<cbe::Object *> &subobjs, NodeIdxType nodeidx) const
{
    SharedLockObjectsDB scopedLock(this);

    std::vector<NodeIdxType> subObjIndices;
    objectTree.getChildren(subObjIndices, nodeidx, true);
    for (NodeIdxType subnodeIdx : subObjIndices)
    {
        if (cbe::Object *obj = getObject(subnodeIdx))
        {
            subobjs.emplace_back(obj);
        }
    }
}

void CoreObjectsDB::getChildren(std::vector<cbe::Object *> &children, NodeIdxType nodeidx) const
{
    SharedLockObjectsDB scopedLock(this);

    std::vector<NodeIdxType> childIndices;
    objectTree.getChildren(childIndices, nodeidx);
    children.reserve(children.size() + childIndices.size());
    for (NodeIdxType subnodeIdx : childIndices)
    {
        if (cbe::Object *obj = getObject(subnodeIdx))
        {
            children.emplace_back(obj);
        }
    }
}
