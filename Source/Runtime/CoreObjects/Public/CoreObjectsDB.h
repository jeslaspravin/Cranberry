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
    const TChar *objectPath = nullptr;
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
    using NodeIdxType = SizeT;
    struct ObjectData
    {
        String path;
        // Below 2 can be used to retrieve object from allocator directly
        CBEClass clazz;
        ObjectAllocIdx allocIdx;
        StringID sid;
    };

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

    ObjectIDToNodeIdx objectIdToNodeIdx;
    ObjectTreeType objectTree;
    SharedLockType *dbLock;

public:
    CoreObjectsDB();
    MAKE_TYPE_NONCOPY_NONMOVE(CoreObjectsDB);
    ~CoreObjectsDB();

    void clear();

    NodeIdxType addObject(StringID objectId, const ObjectData &objData, ObjectsDBQuery &&parentQuery);
    NodeIdxType addRootObject(StringID objectId, const ObjectData &objData);
    // Removes object and all its sub-object from db
    void removeObject(ObjectsDBQuery &&query);
    void removeObject(NodeIdxType nodeIdx);
    void setObject(ObjectsDBQuery &&query, StringID newId, const TChar *newFullPath);
    void setObject(NodeIdxType nodeIdx, StringID newId, const TChar *newFullPath);
    // Invalid newParent clears current parent
    void setObjectParent(ObjectsDBQuery &&query, ObjectsDBQuery &&parentQuery);
    void setObjectParent(NodeIdxType nodeIdx, ObjectsDBQuery &&parentQuery);

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
    FORCE_INLINE bool hasChild(NodeIdxType nodeIdx) const { return objectTree.hasChild(nodeIdx); }
    FORCE_INLINE bool hasChild(ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            return hasChild(itr->second);
        }
        return false;
    }

    cbe::Object *getObject(NodeIdxType nodeIdx) const;
    FORCE_INLINE cbe::Object *getObject(ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            return getObject(itr->second);
        }
        return nullptr;
    }
    FORCE_INLINE NodeIdxType getObjectNodeIdx(ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            return itr->second;
        }
        return ObjectTreeType::InvalidIdx;
    }
    /**
     * All sub objects(Includes entire tree's branch hierarchy under an object
     */
    void getSubobjects(std::vector<NodeIdxType> &subobjNodeIdxs, NodeIdxType nodeIdx) const;
    void getSubobjects(std::vector<cbe::Object *> &subobjs, NodeIdxType nodeIdx) const;
    FORCE_INLINE void getSubobjects(std::vector<cbe::Object *> &subobjs, ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            getSubobjects(subobjs, itr->second);
        }
    }
    void getChildren(std::vector<cbe::Object *> &children, NodeIdxType nodeIdx) const;
    FORCE_INLINE void getChildren(std::vector<cbe::Object *> &children, ObjectsDBQuery &&query) const
    {
        SharedLockObjectsDB scopedLock(this);

        auto itr = findQueryNodeIdx(this, query);
        if (itr != objectIdToNodeIdx.cend())
        {
            getChildren(children, itr->second);
        }
    }

    /**
     * Will be in order such that Root object will be appearing before sub objects
     */
    void getAllObjects(std::vector<cbe::Object *> &outObjects) const;

private:
    FORCE_INLINE bool isMainThread() const;

    struct ObjectClassMatchFilters
    {
        using Filter = Function<bool, const ObjectsDBQuery &, const ObjectData &>;

        FORCE_INLINE static bool ignore(const ObjectsDBQuery &query, const ObjectData &objectData) { return true; }
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
            if (TCharStr::isEqual(objData.path.getChar(), query.objectPath))
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