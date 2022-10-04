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

CoreObjectsDB::NodeIdxType CoreObjectsDB::addObject(StringID objectId, const ObjectData &objData, ObjectsDBQuery &&parentQuery)
{
    fatalAssertf(isMainThread(), "Add object %s must be done from main thread!", objData.path);

    auto parentItr = findQueryNodeIdx(this, parentQuery);
    debugAssert(
        objectId.isValid() && !TCharStr::empty(objData.path.getChar())
        && !hasObject({ .objectPath = objData.path.getChar(), .objectId = objData.sid }) && parentItr != objectIdToNodeIdx.end()
    );

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    NodeIdxType nodeIdx = objectTree.add(objData, parentItr->second);
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

void CoreObjectsDB::removeObject(ObjectsDBQuery &&query)
{
    fatalAssertf(isMainThread(), "Remove object %s must be done from main thread!", query.objectPath);

    auto objItr = findQueryNodeIdx(this, query);
    debugAssert(query.objectId.isValid() && !TCharStr::empty(query.objectPath) && objItr != objectIdToNodeIdx.end());

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    objectTree.remove(objItr->second);
    objectIdToNodeIdx.erase(objItr);
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

void CoreObjectsDB::setObject(ObjectsDBQuery &&query, StringID newId, const TChar *newFullPath)
{
    fatalAssertf(isMainThread(), "Set object %s must be done from main thread!", query.objectPath);

    auto objItr = findQueryNodeIdx(this, query);
    debugAssert(
        query.objectId.isValid() && newId.isValid() && query.objectId != newId
        && !hasObject({ .objectPath = query.objectPath, .clazz = query.clazz, .objectId = newId, .classMatch = query.classMatch })
        && objItr != objectIdToNodeIdx.end()
    );

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    NodeIdxType nodeIdx = objItr->second;
    objectIdToNodeIdx.erase(objItr);
    objectIdToNodeIdx.emplace(newId, nodeIdx);

    ObjectData &objData = objectTree[nodeIdx];
    objData.sid = newId;
    objData.path = newFullPath;
}

void CoreObjectsDB::setObject(NodeIdxType nodeIdx, StringID newId, const TChar *newFullPath)
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
}

void CoreObjectsDB::setObjectParent(ObjectsDBQuery &&query, ObjectsDBQuery &&parentQuery)
{
    fatalAssertf(isMainThread(), "Set parent object %s must be done from main thread!", query.objectPath);

    auto objItr = findQueryNodeIdx(this, query);
    debugAssert(objItr != objectIdToNodeIdx.end());

    SharedLockObjectsDB scopedLock(this);

    setObjectParent(objItr->second, std::forward<ObjectsDBQuery>(parentQuery));
}

void CoreObjectsDB::setObjectParent(NodeIdxType nodeIdx, ObjectsDBQuery &&parentQuery)
{
    fatalAssertf(isMainThread(), "Set parent object for object with node index %s must be done from main thread!", nodeIdx);
    debugAssert(objectTree.isValid(nodeIdx));

    std::scoped_lock<SharedLockType> scopedLock(*dbLock);

    if (parentQuery.objectId.isValid())
    {
        auto parentItr = findQueryNodeIdx(this, parentQuery);
        debugAssert(parentItr != objectIdToNodeIdx.end());
        objectTree.relinkTo(nodeIdx, parentItr->second);
    }
    else
    {
        objectTree.relinkTo(nodeIdx);
    }
}

cbe::Object *CoreObjectsDB::getObject(NodeIdxType nodeIdx) const
{
    SharedLockObjectsDB scopedLock(this);

    if (objectTree.isValid(nodeIdx))
    {
        const ObjectData &objData = objectTree[nodeIdx];
        return cbe::getObjAllocator(objData.clazz)->getAt<cbe::Object>(objData.allocIdx);
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
