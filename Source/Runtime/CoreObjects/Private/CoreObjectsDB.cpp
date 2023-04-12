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

FORCE_INLINE bool CoreObjectsDB::isMainThread() const { return copat::JobSystem::get()->isInThread(copat::EJobThreadType::MainThread); }

void CoreObjectsDB::SharedLockObjectsDB::lockShared() const { objsDb->dbLock->lock_shared(); }
void CoreObjectsDB::SharedLockObjectsDB::unlockShared() const { objsDb->dbLock->unlock_shared(); }

CoreObjectsDB::CoreObjectsDB()
{
    dbLock = new SharedLockType;
    // Start with one hundred thousand elements as norm?
    objectIdToNodeIdx.reserve(100000);
}

CoreObjectsDB::~CoreObjectsDB()
{
    delete dbLock;
    dbLock = nullptr;
}

void CoreObjectsDB::clear()
{
    std::scoped_lock<SharedLockType> scopedLock(*dbLock);
    objectTree.clear();
    objectIdToNodeIdx.clear();
}

CoreObjectsDB::NodeIdxType CoreObjectsDB::addObject(StringID objectId, const ObjectData &objData, NodeIdxType parentNodeIdx)
{
    fatalAssertf(isMainThread(), "Add object %s must be done from main thread!", objData.path);

    debugAssert(
        objectId.isValid() && !TCharStr::empty(objData.path.getChar())
        && !hasObject({ .objectPath = objData.path.getChar(), .objectId = objData.sid })
    );

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    NodeIdxType nodeIdx = objectTree.add(objData, parentNodeIdx);
    objectIdToNodeIdx.emplace(objectId, nodeIdx);
    return nodeIdx;
}

CoreObjectsDB::NodeIdxType CoreObjectsDB::addRootObject(StringID objectId, const ObjectData &objData)
{
    fatalAssertf(isMainThread(), "Add object %s must be done from main thread!", objData.path);
    debugAssert(
        objectId.isValid() && !TCharStr::empty(objData.path.getChar())
        && !hasObject({ .objectPath = objData.path.getChar(), .objectId = objData.sid })
    );

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    NodeIdxType nodeIdx = objectTree.add(objData);
    objectIdToNodeIdx.emplace(objectId, nodeIdx);
    return nodeIdx;
}

void CoreObjectsDB::removeObject(NodeIdxType nodeIdx)
{
    fatalAssertf(isMainThread(), "Remove object at node index %llu must be done from main thread!", nodeIdx);
    debugAssert(objectTree.isValid(nodeIdx));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    const ObjectData &objData = objectTree[nodeIdx];
    objectIdToNodeIdx.erase(findQueryNodeIdx(
        this, { .objectPath = objData.path.getChar(), .clazz = objData.clazz, .objectId = objData.sid, .classMatch = EObjectClassMatch::Exact }
    ));
    objectTree.remove(nodeIdx);
}

void CoreObjectsDB::setObject(NodeIdxType nodeIdx, StringID newId, StringView newFullPath, StringView objName)
{
    fatalAssertf(isMainThread(), "Set object at node index %llu must be done from main thread!", nodeIdx);
    debugAssert(objectTree.isValid(nodeIdx) && newId.isValid());

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    ObjectData &objData = objectTree[nodeIdx];
    objectIdToNodeIdx.erase(findQueryNodeIdx(
        this, { .objectPath = objData.path.getChar(), .clazz = objData.clazz, .objectId = objData.sid, .classMatch = EObjectClassMatch::Exact }
    ));
    objectIdToNodeIdx.emplace(newId, nodeIdx);
    objData.sid = newId;
    objData.path = newFullPath;
    objData.name = objName;
}

void CoreObjectsDB::setObjectParent(NodeIdxType nodeIdx, NodeIdxType parentNodeIdx)
{
    fatalAssertf(isMainThread(), "Set parent object for object with node index %llu must be done from main thread!", nodeIdx);
    debugAssert(objectTree.isValid(nodeIdx));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    objectTree.relinkTo(nodeIdx, parentNodeIdx);
}

cbe::Object *CoreObjectsDB::getObject(NodeIdxType nodeIdx) const
{
    SharedLockObjectsDB scopedLock(this);

    if (objectTree.isValid(nodeIdx))
    {
        const ObjectData &objData = objectTree[nodeIdx];
        if (ANY_BIT_SET(objData.flags, cbe::EObjectFlagBits::ObjFlag_GCPurge))
        {
            cbe::ObjectAllocatorBase *allocator = cbe::getObjAllocator(objData.clazz);
            return allocator && allocator->isValid(objData.allocIdx) ? allocator->getAt<cbe::Object>(objData.allocIdx) : nullptr;
        }
        else
        {
            return cbe::getObjAllocator(objData.clazz)->getAt<cbe::Object>(objData.allocIdx);
        }
    }
    return nullptr;
}

void CoreObjectsDB::getSubobjects(std::vector<NodeIdxType> &subobjNodeIdxs, NodeIdxType nodeIdx) const
{
    SharedLockObjectsDB scopedLock(this);

    objectTree.getChildren(subobjNodeIdxs, nodeIdx, true);
}

void CoreObjectsDB::getSubobjects(std::vector<cbe::Object *> &subobjs, NodeIdxType nodeIdx) const
{
    SharedLockObjectsDB scopedLock(this);

    std::vector<NodeIdxType> subobjNodeIdxs;
    objectTree.getChildren(subobjNodeIdxs, nodeIdx, true);
    for (NodeIdxType subnodeIdx : subobjNodeIdxs)
    {
        if (cbe::Object *obj = getObject(subnodeIdx))
        {
            subobjs.emplace_back(obj);
        }
    }
}

void CoreObjectsDB::getChildren(std::vector<cbe::Object *> &children, NodeIdxType nodeIdx) const
{
    SharedLockObjectsDB scopedLock(this);

    std::vector<NodeIdxType> childIndices;
    objectTree.getChildren(childIndices, nodeIdx);
    children.reserve(children.size() + childIndices.size());
    for (NodeIdxType subnodeIdx : childIndices)
    {
        if (cbe::Object *obj = getObject(subnodeIdx))
        {
            children.emplace_back(obj);
        }
    }
}

void CoreObjectsDB::getAllObjects(std::vector<cbe::Object *> &outObjects) const
{
    SharedLockObjectsDB scopedLock(this);

    outObjects.reserve(objectTree.size());

    std::vector<NodeIdxType> rootIndices;
    objectTree.getAllRoots(rootIndices);
    for (NodeIdxType rootIdx : rootIndices)
    {
        outObjects.emplace_back(getObject(rootIdx));
        getSubobjects(outObjects, rootIdx);
    }
}
