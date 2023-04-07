/*!
 * \file BVH.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Box.h"
#include "Math/Grid.h"
#include "Math/VectorN.h"
#include "Types/Containers/BitArray.h"
#include "Types/Containers/SparseVector.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#define USE_CELLIDX_RANGE_ITR 0

template <typename StorageType>
class BoundingVolume
{
private:
    using GridCellIndex = CellIndex<3>;
    using GridCellIndexRange = CellIndexRange<3>;
    using ObjectStorageType = SparseVector<StorageType, BitArraySparsityPolicy>;
    using ObjectIdxType = ObjectStorageType::size_type;
    using GridCellStorage = std::vector<ObjectIdxType>;

    UniformGrid<Vector3, 3> volumeGrid;
    VectorN<GridCellStorage, 3> grid;

    ObjectStorageType allObjects;
    FORCE_INLINE void addObject(ObjectIdxType objIdx, const GridCellIndex &atIdx);
    FORCE_INLINE void removeObject(ObjectIdxType objIdx, const GridCellIndex &atIdx);
    FORCE_INLINE ObjectIdxType findObjectIndex(const StorageType &object, const AABB &bound) const;

    bool isValidBoundIdxs(const GridCellIndex &minBoundIdx, const GridCellIndex &maxBoundIdx, const AABB &box) const;

public:
    BoundingVolume() = default;
    BoundingVolume(std::vector<StorageType> &&objectList, const Vector3 &cellSize);
    BoundingVolume(const std::vector<StorageType> &objectList, const Vector3 &cellSize);
    ~BoundingVolume();

    void reinitialize(const Vector3 &cellSize);
    void reinitialize(const std::vector<StorageType> &newObjectList, const Vector3 &cellSize);

    void addedNewObject(const StorageType &object);
    void removeAnObject(const StorageType &object);

    template <typename IntersectionSet = std::vector<StorageType>>
    FORCE_INLINE IntersectionSet findIntersection(const AABB &box, bool bSkipObjChecks)
    {
        IntersectionSet intersection;
        findIntersection(intersection, box, bSkipObjChecks);
        return intersection;
    }

    template <typename IntersectionSet = std::vector<StorageType>>
    DEBUG_INLINE void findIntersection(IntersectionSet &intersection, const AABB &box, bool bSkipObjChecks)
    {
        if (!getBounds().intersect(box))
        {
            return;
        }

        struct ObjectIdxHashBypass
        {
            NODISCARD FORCE_INLINE SizeT operator() (ObjectIdxType idx) const noexcept { return idx; }
        };
        BitArray<uint64> resultIdxAdded(allObjects.totalCount());
        std::vector<ObjectIdxType> resultIdxs;
        resultIdxs.reserve(allObjects.size());

        GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(box.minBound)));
        GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(box.maxBound)));
#if USE_CELLIDX_RANGE_ITR
        for (const GridCellIndex &currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
        {
            const GridCellStorage &objs = grid[currentIdx];
            if (bSkipObjChecks)
            {
                for (const ObjectIdxType &objIdx : objs)
                {
                    debugAssert(allObjects.isValid(objIdx));
                    if (!resultIdxAdded[objIdx])
                    {
                        resultIdxs.emplace_back(objIdx);
                        resultIdxAdded[objIdx] = true;
                    }
                }
            }
            else
            {
                for (const ObjectIdxType &objIdx : objs)
                {
                    debugAssert(allObjects.isValid(objIdx));
                    if (!resultIdxAdded[objIdx] && box.intersect(allObjects[objIdx].getBounds()))
                    {
                        resultIdxs.emplace_back(objIdx);
                        resultIdxAdded[objIdx] = true;
                    }
                }
            }
        }
#else  // USE_CELLIDX_RANGE_ITR
        GridCellIndex currentIdx;
        for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
        {
            for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
            {
                for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
                {
                    const GridCellStorage &objs = grid[currentIdx];
                    if (bSkipObjChecks)
                    {
                        for (const ObjectIdxType &objIdx : objs)
                        {
                            debugAssert(allObjects.isValid(objIdx));
                            if (!resultIdxAdded[objIdx])
                            {
                                resultIdxs.emplace_back(objIdx);
                                resultIdxAdded[objIdx] = true;
                            }
                        }
                    }
                    else
                    {
                        for (const ObjectIdxType &objIdx : objs)
                        {
                            debugAssert(allObjects.isValid(objIdx));
                            if (!resultIdxAdded[objIdx] && box.intersect(allObjects[objIdx].getBounds()))
                            {
                                resultIdxs.emplace_back(objIdx);
                                resultIdxAdded[objIdx] = true;
                            }
                        }
                    }
                }
            }
        }
#endif // USE_CELLIDX_RANGE_ITR

        for (const ObjectIdxType &objIdx : resultIdxs)
        {
            debugAssert(allObjects.isValid(objIdx));
            intersection.emplace_back(allObjects[objIdx]);
        }
    }

    bool raycast(std::vector<StorageType> &result, const Vector3 &start, const Vector3 &dir, float length, bool bExistOnHit = true);

    void updateBounds(const StorageType &object, const AABB &oldBox, const AABB &newBox);
    bool isSameBounds(const AABB &boxOne, const AABB &boxTwo);
    AABB getBounds() const;
};

template <typename StorageType>
void BoundingVolume<StorageType>::removeObject(ObjectIdxType objIdx, const GridCellIndex &atIdx)
{
    GridCellStorage &objects = grid[atIdx];
    std::erase(objects, objIdx);
}

template <typename StorageType>
void BoundingVolume<StorageType>::addObject(ObjectIdxType objIdx, const GridCellIndex &atIdx)
{
    GridCellStorage &objects = grid[atIdx];
#if DEBUG_BUILD
    alertAlwaysf(std::find(objects.cbegin(), objects.cend(), objIdx) == objects.cend(), "Object of index %llu duplicate insertion", objIdx);
#endif
    objects.emplace_back(objIdx);
}

template <typename StorageType>
BoundingVolume<StorageType>::ObjectIdxType BoundingVolume<StorageType>::findObjectIndex(const StorageType &object, const AABB &bound) const
{
    debugAssert(getBounds().intersect(bound));

    ObjectIdxType foundIdx = allObjects.totalCount();
    auto objectFinderPredicate = [this, &object](ObjectIdxType objIdx)
    {
        return allObjects.isValid(objIdx) && allObjects[objIdx] == object;
    };

    GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(bound.minBound)));
    GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(bound.maxBound)));
#if USE_CELLIDX_RANGE_ITR
    for (const GridCellIndex &currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
    {
        const GridCellStorage &objs = grid[currentIdx];
        auto itr = std::find_if(objs.cbegin(), objs.cend(), objectFinderPredicate);
        if (itr != objs.cend())
        {
            foundIdx = *itr;
            break;
        }
    }
#else  // USE_CELLIDX_RANGE_ITR
    GridCellIndex currentIdx;
    for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
    {
        for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
        {
            for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
            {
                const GridCellStorage &objs = grid[currentIdx];
                auto itr = std::find_if(objs.cbegin(), objs.cend(), objectFinderPredicate);
                if (itr != objs.cend())
                {
                    foundIdx = *itr;
                    break;
                }
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR
    return foundIdx;
}

template <typename StorageType>
bool BoundingVolume<StorageType>::isValidBoundIdxs(const GridCellIndex &minBoundIdx, const GridCellIndex &maxBoundIdx, const AABB &box) const
{
    // if any dimension is clamped because of being outside the volume bound
    bool bIsAnyDimensionClamped = false;
    Vector3 boxExtend = box.size();
    for (int32 i = 0; i < 3; ++i)
    {
        uint32 cellExtend = maxBoundIdx[i] - minBoundIdx[i];
        // Invalid cell extend in volume bound while box is not thin plane
        bIsAnyDimensionClamped = bIsAnyDimensionClamped || (cellExtend == 0 && !Math::isEqual(boxExtend[i], 0.0f));
    }
    return !bIsAnyDimensionClamped;
}

template <typename StorageType>
AABB BoundingVolume<StorageType>::getBounds() const
{
    AABB retVal;
    volumeGrid.getBound(retVal.minBound, retVal.maxBound);
    return retVal;
}

template <typename StorageType>
BoundingVolume<StorageType>::BoundingVolume(std::vector<StorageType> &&objectList, const Vector3 &cellSize)
{
    allObjects = std::move(objectList);
    reinitialize(cellSize);
}

template <typename StorageType>
BoundingVolume<StorageType>::BoundingVolume(const std::vector<StorageType> &objectList, const Vector3 &cellSize)
{
    allObjects = objectList;
    reinitialize(cellSize);
}

template <typename StorageType>
BoundingVolume<StorageType>::~BoundingVolume()
{
    allObjects.clear();
    grid.clear();
}

template <typename StorageType>
void BoundingVolume<StorageType>::reinitialize(const Vector3 &cellSize)
{
    if (allObjects.size() <= 0)
    {
        return;
    }

    AABB globalBound;

    for (const StorageType &obj : allObjects)
    {
        const AABB &bound = obj.getBounds();
        globalBound.grow(bound);
    }

    LOG_DEBUG(
        "BVH", "Before correcting cell size global bounding box size is (%f, %f, %f)", globalBound.size().x(), globalBound.size().y(),
        globalBound.size().z()
    );

    volumeGrid.InitWithSize(globalBound.minBound, globalBound.maxBound, cellSize);
    volumeGrid.getBound(globalBound.minBound, globalBound.maxBound);

    LOG_DEBUG(
        "BVH", "After correcting cell size global bounding box size is (%f, %f, %f)", globalBound.size().x(), globalBound.size().y(),
        globalBound.size().z()
    );

    GridCellIndex cellCount = volumeGrid.cellCount();
    grid = VectorN<GridCellStorage, 3>(cellCount);

    for (ObjectIdxType objIdx = 0; objIdx < allObjects.totalCount(); ++objIdx)
    {
        debugAssert(allObjects.isValid(objIdx));
        const AABB &bound = allObjects[objIdx].getBounds();
        GridCellIndex minBoundIdx = volumeGrid.cell(bound.minBound);
        GridCellIndex maxBoundIdx = volumeGrid.cell(bound.maxBound);

#if USE_CELLIDX_RANGE_ITR
        for (const GridCellIndex &currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
        {
            addObject(objIdx, currentIdx);
        }
#else  // USE_CELLIDX_RANGE_ITR
        GridCellIndex currentIdx;
        for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
        {
            for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
            {
                for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
                {
                    // LOG_DEBUG("BVH", "Adding %d to (%d,%d,%d)", obj,
                    // x, y, z);
                    addObject(objIdx, currentIdx);
                }
            }
        }
#endif // USE_CELLIDX_RANGE_ITR
    }
}

template <typename StorageType>
void BoundingVolume<StorageType>::reinitialize(const std::vector<StorageType> &newObjectList, const Vector3 &cellSize)
{
    allObjects = newObjectList;
    reinitialize(cellSize);
}

template <typename StorageType>
void BoundingVolume<StorageType>::addedNewObject(const StorageType &object)
{
    const AABB &objBound = object.getBounds();
    ObjectIdxType objIdx = allObjects.get(object);

    {
        GridCellIndex newCellCount = volumeGrid.cellCount();
        Vector3 newMinCorner;
        Vector3 newMaxCorner;
        volumeGrid.getBound(newMinCorner, newMaxCorner);
        Vector3 currMinCorner = newMinCorner;
        Vector3 currMaxCorner = newMaxCorner;

        bool bChanged = false;
        for (uint32 axis = 0; axis < 3; ++axis)
        {
            if (objBound.minBound[axis] < currMinCorner[axis])
            {
                uint32 numNewCells = (uint32)((currMinCorner[axis] - objBound.minBound[axis]) / volumeGrid.cellSize()[axis]);
                newCellCount.idx[axis] += numNewCells;
                newMinCorner[axis] -= numNewCells * volumeGrid.cellSize()[axis];
                bChanged = true;
            }
            if (objBound.maxBound[axis] > currMaxCorner[axis])
            {
                uint32 numNewCells = (uint32)((objBound.maxBound[axis] - currMaxCorner[axis]) / volumeGrid.cellSize()[axis]);
                newCellCount.idx[axis] += numNewCells;
                newMaxCorner[axis] += numNewCells * volumeGrid.cellSize()[axis];
                bChanged = true;
            }
        }
        if (bChanged)
        {
            VectorN<GridCellStorage, 3> newElements(newCellCount);
            UniformGrid<Vector3, 3> newGrid;
            newGrid.InitWithCount(newMinCorner, newMaxCorner, newCellCount);
            for (uint32 i = 0; i < volumeGrid.cellCount().size(); ++i)
            {
                GridCellIndex gridIndex = newGrid.cell(volumeGrid.center(i));
                newElements[gridIndex] = grid[volumeGrid.getNdIndex(i)];
            }
            volumeGrid.InitWithCount(newMinCorner, newMaxCorner, newCellCount);
            grid = std::move(newElements);
        }
    }

    GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(objBound.minBound));
    GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(objBound.maxBound));

#if USE_CELLIDX_RANGE_ITR
    for (const GridCellIndex &currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
    {
        addObject(objIdx, currentIdx);
    }
#else  // USE_CELLIDX_RANGE_ITR
    GridCellIndex currentIdx;
    for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
    {
        for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
        {
            for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
            {
                addObject(objIdx, currentIdx);
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR
}

template <typename StorageType>
void BoundingVolume<StorageType>::removeAnObject(const StorageType &object)
{
    const AABB &objBound = object.getBounds();
    if (!getBounds().intersect(objBound))
    {
        return;
    }
    ObjectIdxType objIdx = findObjectIndex(object, objBound);
    debugAssert(allObjects.isValid(objIdx));

    GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(objBound.minBound)));
    GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(objBound.maxBound)));

#if USE_CELLIDX_RANGE_ITR
    for (const GridCellIndex &currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
    {
        removeObject(objIdx, currentIdx);
    }
#else  // USE_CELLIDX_RANGE_ITR
    GridCellIndex currentIdx;
    for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
    {
        for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
        {
            for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
            {
                removeObject(objIdx, currentIdx);
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR

    allObjects.reset(objIdx);
}

template <typename StorageType>
bool BoundingVolume<StorageType>::raycast(
    std::vector<StorageType> &result, const Vector3 &start, const Vector3 &dir, float length, bool bExitOnHit /* = true */
)
{
    AABB globalBound;
    volumeGrid.getBound(globalBound.minBound, globalBound.maxBound);

    Vector3 invDir;
    float invLength = 1.0f / length;

    bool parallel[3];
    for (uint32 i = 0; i < 3; i++)
    {
        parallel[i] = (dir[i] == 0);
        invDir[i] = parallel[i] ? 0 : 1 / dir[i];
    }

    float enterAtLengthFrac, exitAtLengthFrac;
    Vector3 nextStart, nextExit;

    Vector3 tmpPos;

    bool cellsInPath = globalBound.raycastFast(
        start, dir, invDir, length, invLength, &parallel[0], enterAtLengthFrac, nextStart, exitAtLengthFrac, nextExit
    );

    std::vector<ObjectIdxType> resultIdxs;
    resultIdxs.reserve(allObjects.size());
    if (cellsInPath)
    {
        BitArray<uint64> resultIdxAdded(allObjects.totalCount());

        Vector3 halfCellSize = volumeGrid.cellSize() * 0.5f;
        GridCellIndex nextCellIdx = volumeGrid.cell(nextStart);
        nextCellIdx = volumeGrid.clampCellIndex(nextCellIdx);
        float leftLength = length;

        while (leftLength > 0)
        {
            const GridCellStorage &objs = grid[nextCellIdx];
            for (const ObjectIdxType &objIdx : objs)
            {
                debugAssert(allObjects.isValid(objIdx));
                const StorageType &obj = allObjects[objIdx];
                const AABB &objBound = obj.getBounds();

                if (objBound.raycastFast(
                        start, dir, invDir, length, invLength, &parallel[0], enterAtLengthFrac, tmpPos, exitAtLengthFrac, nextExit
                    )
                    && !resultIdxAdded[objIdx])
                {
                    resultIdxs.emplace_back(objIdx);
                    resultIdxAdded[objIdx] = true;

                    if (bExitOnHit)
                    {
                        break;
                    }
                }
            }
            if (bExitOnHit && !resultIdxs.empty())
            {
                break;
            }

            Vector3 cellCenter = volumeGrid.location(nextCellIdx);

            float timesPerAxis[3];
            float bestTime = leftLength;

            bool bReachedEnd = true;
            for (uint32 axis = 0; axis < 3; axis++)
            {
                if (!parallel[axis])
                {
                    const float crossPlanePt = dir[axis] > 0 ? cellCenter[axis] + halfCellSize[axis] : cellCenter[axis] - halfCellSize[axis];
                    const float distance = crossPlanePt - nextStart[axis];
                    const float time = distance * invDir[axis];
                    timesPerAxis[axis] = time;
                    if (bestTime > time)
                    {
                        bestTime = time;
                        bReachedEnd = false;
                    }
                }
                else
                {
                    timesPerAxis[axis] = FLT_MAX;
                }
            }

            if (bReachedEnd)
            {
                break;
            }

            for (uint32 axis = 0; axis < 3; axis++)
            {
                if (parallel[axis])
                {
                    continue;
                }
                nextCellIdx.idx[axis] += (timesPerAxis[axis] <= bestTime + SMALL_EPSILON) ? (dir[axis] > 0) ? 1 : -1 : 0;
                if (nextCellIdx[axis] >= volumeGrid.cellCount()[axis] || nextCellIdx[axis] < 0)
                {
                    bReachedEnd = true;
                    break;
                }
            }

            if (bReachedEnd)
            {
                break;
            }

            nextStart += dir * bestTime;
            leftLength -= (dir * bestTime).length();
        }
    }
    for (const ObjectIdxType &objIdx : resultIdxs)
    {
        result.emplace_back(allObjects[objIdx]);
    }

    return result.size() > 0;
}

template <typename StorageType>
void BoundingVolume<StorageType>::updateBounds(const StorageType &object, const AABB &oldBox, const AABB &newBox)
{
    // If not present already then we must add new object
    if (!getBounds().intersect(oldBox))
    {
        addedNewObject(object);
        return;
    }

    ObjectIdxType objIdx = findObjectIndex(object, oldBox);
    debugAssert(allObjects.isValid(objIdx));

    GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(oldBox.minBound)));
    GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(oldBox.maxBound)));

#if USE_CELLIDX_RANGE_ITR
    for (const GridCellIndex &currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
    {
        removeObject(objIdx, currentIdx);
    }
#else  // USE_CELLIDX_RANGE_ITR
    GridCellIndex currentIdx;
    for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
    {
        for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
        {
            for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
            {
                removeObject(objIdx, currentIdx);
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR

    if (getBounds().intersect(newBox))
    {
        minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(newBox.minBound)));
        maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(newBox.maxBound)));

#if USE_CELLIDX_RANGE_ITR
        for (const GridCellIndex &currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
        {
            addObject(objIdx, currentIdx);
        }
#else  // USE_CELLIDX_RANGE_ITR
        for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
        {
            for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
            {
                for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
                {
                    addObject(objIdx, currentIdx);
                }
            }
        }
#endif // USE_CELLIDX_RANGE_ITR
    }
}

template <typename StorageType>
bool BoundingVolume<StorageType>::isSameBounds(const AABB &boxOne, const AABB &boxTwo)
{
    GridCellIndex minBoundIdx1 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxOne.minBound)));
    GridCellIndex maxBoundIdx1 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxOne.maxBound)));

    GridCellIndex minBoundIdx2 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxTwo.minBound)));
    GridCellIndex maxBoundIdx2 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxTwo.maxBound)));

    return minBoundIdx1 == minBoundIdx2 && maxBoundIdx1 == maxBoundIdx2;
}

#undef USE_CELLIDX_RANGE_ITR