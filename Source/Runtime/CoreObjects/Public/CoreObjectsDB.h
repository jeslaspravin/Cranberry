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

#include "CoreObjectsExports.h"
#include "CBEObjectTypes.h"
#include "String/StringID.h"
#include "Reflections/Functions.h"
#include "Types/Containers/FlatTree.h"
#include "Property/PropertyHelper.h"

namespace std
{
class shared_mutex;
}

enum class EObjectClassMatch
{
    Ignore,
    Exact,
    DerivedFrom
};

struct ObjectsDBQuery
{
    StringView objectPath;
    CBEClass clazz = nullptr;
    StringID objectId;
    // Why ignore? Because even though we allow multiple object with same name now it is not encouraged
    EObjectClassMatch classMatch = EObjectClassMatch::Ignore;
};

/**
 * CoreObjectsDB
 *
 * Contains objects hierarchy data and object SID, Alloc idx in database separate from class for quick
 * access Possible use is for garbage collector in future
 */
class COREOBJECTS_EXPORT CoreObjectsDB
{
public:
    using NodeIdxType = ObjectDbIdx;
    struct ObjectData
    {
        String path;
        EObjectFlags flags = 0;
        // Below 2 can be used to retrieve object from allocator directly
        CBEClass clazz;
        ObjectAllocIdx allocIdx = 0;
        // Offset of name start index in path
        uint32 nameOffset = 0;
        StringID sid;
    };

    // This is just to avoid including std::mutex header
    class COREOBJECTS_EXPORT SharedLockObjectsDB
    {
    private:
        const CoreObjectsDB *objsDb;
        bool bIsLocked = false;

    public:
        FORCE_INLINE SharedLockObjectsDB(const CoreObjectsDB *inDb);
        ~SharedLockObjectsDB()
        {
            if (bIsLocked)
            {
                unlockShared();
            }
        }

    private:
        void lockShared() const;
        void unlockShared() const;
    };

private:
    using SharedLockType = std::shared_mutex;
    using ObjectIDToNodeIdx = std::unordered_multimap<StringID, NodeIdxType>;
    using ObjectTreeType = FlatTree<ObjectData, NodeIdxType>;
    using ObjectPrivateDataView = cbe::ObjectPrivateDataView;

    ObjectIDToNodeIdx objectIdToNodeIdx;
    ObjectTreeType objectTree;
    SharedLockType *dbLock;

public:
    static constexpr const NodeIdxType InvalidDbIdx = ObjectTreeType::InvalidIdx;

public:
    CoreObjectsDB();
    MAKE_TYPE_NONCOPY_NONMOVE(CoreObjectsDB)
    ~CoreObjectsDB();

    void clear();

    NodeIdxType addObject(StringID objectId, StringView fullPath, StringView objName, CBEClass clazz, NodeIdxType parentNodeIdx);
    NodeIdxType addRootObject(StringID objectId, StringView fullPath, StringView objName, CBEClass clazz);
    // Removes object and all its sub-object from db
    void removeObject(NodeIdxType nodeIdx);
    void setObject(NodeIdxType nodeIdx, StringID newId, StringView newFullPath, StringView objName);
    // Invalid newParent clears current parent
    void setObjectParent(NodeIdxType nodeIdx, NodeIdxType parentNodeIdx);
    // Assumes that node index is valid
    void setAllocIdx(NodeIdxType nodeIdx, ObjectAllocIdx allocIdx)
    {
        SharedLockObjectsDB scopedLock(this);
        fatalAssertf(isMainThread(), "Set allocIdx for object with node index %llu must be done from main thread!", nodeIdx);
        debugAssert(objectTree.isValid(nodeIdx));

        objectTree[nodeIdx].allocIdx = allocIdx;
    }
    // Assumes that node index is valid
    EObjectFlags &objectFlags(NodeIdxType nodeIdx)
    {
        SharedLockObjectsDB scopedLock(this);
        debugAssert(objectTree.isValid(nodeIdx));
        return objectTree[nodeIdx].flags;
    }

    // Only determines if the object is present in the database. During GCPurge objects might be here but alloc might not be valid
    FORCE_INLINE bool hasObject(ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            debugAssert(objectTree.isValid(itr->second));
            return objectTree.isValid(itr->second);
        }
        return false;
    }
    FORCE_INLINE bool hasObject(NodeIdxType nodeIdx) const
    {
        SharedLockObjectsDB scopedLock(this);

        if (objectTree.isValid(nodeIdx))
        {
#if DEV_BUILD
            const ObjectData &objData = objectTree[nodeIdx];
            ObjectsDBQuery query{ .objectPath = objData.path.getChar(), .clazz = objData.clazz, .objectId = objData.sid };
            auto itr = findQueryNodeIdx(this, query);
            debugAssert(itr != objectIdToNodeIdx.cend());
            return itr != objectIdToNodeIdx.cend();
#else
            return true;
#endif
        }
        return false;
    }

    cbe::Object *getObject(NodeIdxType nodeIdx) const;
    cbe::Object *getObject(ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            return getObject(itr->second);
        }
        return nullptr;
    }
    NodeIdxType getObjectNodeIdx(ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            return itr->second;
        }
        return InvalidDbIdx;
    }
    ObjectPrivateDataView getObjectData(NodeIdxType nodeIdx) const
    {
        SharedLockObjectsDB scopedLock(this);
        if (!objectTree.isValid(nodeIdx))
        {
            return ObjectPrivateDataView::getInvalid();
        }

        const ObjectData &objData = objectTree[nodeIdx];
        const ObjectTreeType::Node &node = objectTree.getNode(nodeIdx);
        return ObjectPrivateDataView{ .name = objData.path.getChar() + objData.nameOffset,
                                      .path = objData.path.getChar(),
                                      .flags = objData.flags,
                                      .outerIdx = node.parent,
                                      .sid = objData.sid,
                                      .allocIdx = objData.allocIdx,
                                      .clazz = objData.clazz };
    }

    NodeIdxType getParentIdx(NodeIdxType nodeIdx) const
    {
        SharedLockObjectsDB scopedLock(this);
        if (objectTree.isValid(nodeIdx))
        {
            return objectTree.getNode(nodeIdx).parent;
        }
        return InvalidDbIdx;
    }

    FORCE_INLINE bool hasChild(NodeIdxType nodeIdx) const
    {
        SharedLockObjectsDB scopedLock(this);
        return objectTree.hasChild(nodeIdx);
    }
    /**
     * All sub objects(Includes entire tree's branch hierarchy under an object
     */
    void getSubobjects(std::vector<NodeIdxType> &subobjNodeIdxs, NodeIdxType nodeIdx) const;
    void getSubobjects(std::vector<cbe::Object *> &subobjs, NodeIdxType nodeIdx) const;
    void getChildren(std::vector<cbe::Object *> &children, NodeIdxType nodeIdx) const;

    /**
     * Will be in order such that Root object will be appearing before sub objects
     */
    void getAllObjects(std::vector<cbe::Object *> &outObjects) const;

private:
    FORCE_INLINE bool isMainThread() const;

    struct ObjectClassMatchFilters
    {
        using Filter = Function<bool, const ObjectsDBQuery &, const ObjectData &>;

        FORCE_INLINE static bool ignore(const ObjectsDBQuery & /*query*/, const ObjectData & /*objectData*/) { return true; }
        FORCE_INLINE static bool derived(const ObjectsDBQuery &query, const ObjectData &objectData)
        {
            return PropertyHelper::isChildOf(objectData.clazz, query.clazz);
        }
        FORCE_INLINE static bool exact(const ObjectsDBQuery &query, const ObjectData &objectData) { return query.clazz == objectData.clazz; }
    };

    template <typename ConstAwareType>
    static auto findQueryNodeIdx(ConstAwareType *db, const ObjectsDBQuery &query)
    {
        ObjectClassMatchFilters::Filter classFilter;
        switch (query.classMatch)
        {
        case EObjectClassMatch::Exact:
            classFilter = ObjectClassMatchFilters::exact;
            break;
        case EObjectClassMatch::DerivedFrom:
            classFilter = ObjectClassMatchFilters::derived;
            break;
        case EObjectClassMatch::Ignore:
        default:
            classFilter = ObjectClassMatchFilters::ignore;
            break;
        }

        bool bDuplicatePathFound = false;
        auto itrPair = db->objectIdToNodeIdx.equal_range(query.objectId);
        for (auto itr = itrPair.first; itr != itrPair.second; ++itr)
        {
            if (!db->objectTree.isValid(itr->second))
            {
                continue;
            }
            const ObjectData &objData = db->objectTree[itr->second];
            if (objData.path.isEqual(query.objectPath))
            {
                alertAlwaysf(!bDuplicatePathFound, "Objects with duplicate names found %s", objData.path);
                bDuplicatePathFound = true;
                if (classFilter(query, objData))
                {
                    return itr;
                }
            }
        }
        return db->objectIdToNodeIdx.end();
    }
};

FORCE_INLINE CoreObjectsDB::SharedLockObjectsDB::SharedLockObjectsDB(const CoreObjectsDB *inDb)
    : objsDb(inDb)
    , bIsLocked(false)
{
    if (!objsDb->isMainThread())
    {
        bIsLocked = true;
        lockShared();
    }
}