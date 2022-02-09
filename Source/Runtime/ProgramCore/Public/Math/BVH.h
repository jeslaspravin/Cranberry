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

#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/CoreDefines.h"
#include "Math/Grid.h"
#include "Math/Box.h"
#include "Math/VectorND.h"

#include <list>

#define USE_CELLIDX_RANGE_ITR 0

template <typename StorageType>
class BoundingVolume 
{
private:
    using GridCellIndex = CellIndex<3>;
    using GridCellIndexRange = CellIndexRange<3>;

    UniformGrid<Vector3D, 3> volumeGrid;
    VectorND<std::list<StorageType>, 3> grid;

    std::list<StorageType> allObjects;
    FORCE_INLINE void addObject(const StorageType& obj, const GridCellIndex& atIdx);
    FORCE_INLINE void removeObject(const StorageType& obj, const GridCellIndex& atIdx);

    bool isValidBoundIdxs(const GridCellIndex& minBoundIdx, const GridCellIndex& maxBoundIdx, const AABB& box) const;
public:

    BoundingVolume() = default;
    BoundingVolume(std::list<StorageType>&& objectList, const Vector3D& cellSize);
    BoundingVolume(const std::list<StorageType>& objectList, const Vector3D& cellSize);
    ~BoundingVolume();

    void reinitialize(const Vector3D& cellSize);
    void reinitialize(const std::list<StorageType>& newObjectList, const Vector3D& cellSize);

    void addedNewObject(const StorageType& object);
    void removeAnObject(const StorageType& object);

    template<typename IntersectionSet = std::set<StorageType>>
    FORCE_INLINE IntersectionSet findIntersection(const AABB& box, bool bSkipObjChecks)
    {
        IntersectionSet intersection;
        findIntersection(intersection, box, bSkipObjChecks);
        return intersection;
    }

    template<typename IntersectionSet = std::set<StorageType>>
    DEBUG_INLINE void findIntersection(IntersectionSet& intersection, const AABB& box, bool bSkipObjChecks)
    {
        if (!getBounds().intersect(box))
        {
            return;
        }

        GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(
            volumeGrid.cell(
                volumeGrid.clampLocation(box.minBound)));
        GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(
            volumeGrid.cell(
                volumeGrid.clampLocation(box.maxBound)));
#if USE_CELLIDX_RANGE_ITR
        for (const GridCellIndex& currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
        {
            const std::list<StorageType>& objs = grid[currentIdx];
            if (bSkipObjChecks)
            {
                for (const StorageType& obj : objs)
                {
                    intersection.insert(obj);
                }
                // Above is faster than below, as below assumes that begin and end denotes sorted list
                //intersection.insert(objs.cbegin(), objs.cend());
            }
            else
            {
                for (const StorageType& obj : objs)
                {
                    const AABB& bound = obj.getBounds();
                    if (box.intersect(bound))
                    {
                        intersection.insert(obj);
                    }
                }
            }
        }
#else // USE_CELLIDX_RANGE_ITR
        GridCellIndex currentIdx;
        for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
        {
            for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
            {
                for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
                {
                    const std::list<StorageType>& objs = grid[currentIdx];
                    if (bSkipObjChecks)
                    {
                        for (const StorageType& obj : objs)
                        {
                            intersection.insert(obj);
                        }
                        // Above is faster than below, as below assumes that begin and end denotes sorted list
                        //intersection.insert(objs.cbegin(), objs.cend());
                    }
                    else
                    {
                        for (const StorageType& obj : objs)
                        {
                            const AABB& bound = obj.getBounds();
                            if (box.intersect(bound))
                            {
                                intersection.insert(obj);
                            }
                        }
                    }
                }
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR

    bool raycast(const Vector3D& start, const Vector3D& dir, const float& length, std::vector<StorageType>& result, bool bExistOnHit = true);

    bool updateBoundsChecked(const StorageType& object, const AABB& oldBox, const AABB& newBox);
    void updateBounds(const StorageType& object, const AABB& oldBox, const AABB& newBox);
    bool isSameBounds(const AABB& boxOne, const AABB& boxTwo);
    AABB getBounds() const;
};

template <typename StorageType>
void BoundingVolume<StorageType>::removeObject(const StorageType& obj, const GridCellIndex& atIdx)
{
    grid[atIdx].remove(obj);
}

template <typename StorageType>
void BoundingVolume<StorageType>::addObject(const StorageType& obj, const GridCellIndex& atIdx)
{
    std::list<StorageType>& objects = grid[atIdx];
#if 0 // Sorted insertion is not changing much performance
    auto itr = objects.begin();
    auto nextItr = itr;
    if (itr != objects.end())
    {
        nextItr++;
    }
    // Sorted insert
    while (nextItr != objects.end())
    {
        if (*itr < obj && (obj < *nextItr || obj == *nextItr))
        {
            break;
        }
        itr = nextItr;
        nextItr++;
    }

    if (nextItr == objects.end() || !(*nextItr == obj))
    {
        objects.insert(nextItr, obj);
    }
#endif
    objects.emplace_back(obj);
}

template <typename StorageType>
bool BoundingVolume<StorageType>::isValidBoundIdxs(const GridCellIndex& minBoundIdx, const GridCellIndex& maxBoundIdx, const AABB& box) const
{
    // if any dimension is clamped because of being outside the volume bound
    bool bIsAnyDimensionClamped = false;
    Vector3D boxExtend = box.size();
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
BoundingVolume<StorageType>::BoundingVolume(std::list<StorageType>&& objectList, const Vector3D& cellSize)
{
    allObjects = objectList;
    reinitialize(cellSize);
}

template <typename StorageType>
BoundingVolume<StorageType>::BoundingVolume(const std::list<StorageType>& objectList, const Vector3D& cellSize)
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
void BoundingVolume<StorageType>::reinitialize(const Vector3D& cellSize)
{
    if (allObjects.size() <= 0)
        return;

    AABB globalBound;

    for (const StorageType& obj : allObjects)
    {
        const AABB& bound = obj.getBounds();
        globalBound.grow(bound);
    }

    LOG_DEBUG("BVH", "%s() : Before correcting cell size global bounding box size is (%f, %f, %f)", __func__, globalBound.size().x(), globalBound.size().y(), globalBound.size().z());

    volumeGrid.InitWithSize(globalBound.minBound, globalBound.maxBound, cellSize);
    volumeGrid.getBound(globalBound.minBound, globalBound.maxBound);

    LOG_DEBUG("BVH", "%s() : After correcting cell size global bounding box size is (%f, %f, %f)", __func__, globalBound.size().x(), globalBound.size().y(), globalBound.size().z());

    GridCellIndex cellCount = volumeGrid.cellCount();
    grid = VectorND<std::list<StorageType>, 3>(cellCount);

    for (const StorageType& obj : allObjects)
    {
        const AABB& bound = obj.getBounds();
        GridCellIndex minBoundIdx = volumeGrid.cell(bound.minBound);
        GridCellIndex maxBoundIdx = volumeGrid.cell(bound.maxBound);

#if USE_CELLIDX_RANGE_ITR
        for (const GridCellIndex& currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
        {
            addObject(obj, currentIdx);
        }
#else // USE_CELLIDX_RANGE_ITR
        GridCellIndex currentIdx;
        for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
        {
            for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
            {
                for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
                {
                    // LOG_DEBUG("BVH", "%s() : Adding %d to (%d,%d,%d)", __func__, obj, x, y, z);
                    addObject(obj, currentIdx);
                }
            }
        }
#endif // USE_CELLIDX_RANGE_ITR
    }

}

template <typename StorageType>
void BoundingVolume<StorageType>::reinitialize(const std::list<StorageType>& newObjectList, const Vector3D& cellSize)
{
    allObjects = newObjectList;
    reinitialize(cellSize);
}

template <typename StorageType>
void BoundingVolume<StorageType>::addedNewObject(const StorageType& object)
{
    const AABB& objBound = object.getBounds();

    {
        GridCellIndex newCellCount = volumeGrid.cellCount();
        Vector3D newMinCorner;
        Vector3D newMaxCorner;
        volumeGrid.getBound(newMinCorner, newMaxCorner);
        Vector3D currMinCorner = newMinCorner;
        Vector3D currMaxCorner = newMaxCorner;

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
            VectorND<std::list<StorageType>, 3> newElements(newCellCount);
            UniformGrid<Vector3D, 3> newGrid;
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
    for (const GridCellIndex& currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
    {
        addObject(object, currentIdx);
    }
#else // USE_CELLIDX_RANGE_ITR
    GridCellIndex currentIdx;
    for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
    {
        for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
        {
            for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
            {
                addObject(object, currentIdx);
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR

    allObjects.push_back(object);
}

template <typename StorageType>
void BoundingVolume<StorageType>::removeAnObject(const StorageType& object)
{
    const AABB& objBound = object.getBounds();

    GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(objBound.minBound)));
    GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(objBound.maxBound)));

#if USE_CELLIDX_RANGE_ITR
    for (const GridCellIndex& currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
    {
        removeObject(object, currentIdx);
    }
#else // USE_CELLIDX_RANGE_ITR
    GridCellIndex currentIdx;
    for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
    {
        for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
        {
            for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
            {
                removeObject(object, currentIdx);
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR

    allObjects.remove(object);
}

template <typename StorageType>
bool BoundingVolume<StorageType>::raycast(const Vector3D& start, const Vector3D& dir, const float& length, std::vector<StorageType>& result, bool bExitOnHit/* = true */)
{
    AABB globalBound;
    volumeGrid.getBound(globalBound.minBound, globalBound.maxBound);

    Vector3D invDir;
    float invLength = 1.0f / length;

    bool parallel[3];
    for (uint32 i = 0; i < 3; i++)
    {
        parallel[i] = (dir[i] == 0);
        invDir[i] = parallel[i] ? 0 : 1 / dir[i];
    }

    float enterAtLengthFrac, exitAtLengthFrac;
    Vector3D nextStart, nextExit;

    Vector3D tmpPos;

    bool cellsInPath = globalBound.raycastFast(start, dir, invDir, length, invLength, &parallel[0], enterAtLengthFrac,
        nextStart, exitAtLengthFrac, nextExit);

    if (cellsInPath) 
    {
        Vector3D halfCellSize = volumeGrid.cellSize() * 0.5f;
        GridCellIndex nextCellIdx = volumeGrid.cell(nextStart);
        nextCellIdx = volumeGrid.clampCellIndex(nextCellIdx);
        float leftLength = length;

        while (leftLength > 0)
        {
            const std::list<StorageType>& objs = grid[nextCellIdx];

            for (const StorageType& obj : objs)
            {
                const AABB& objBound = obj.getBounds();

                if (objBound.raycastFast(start, dir, invDir, length, invLength, &parallel[0], enterAtLengthFrac,
                    tmpPos, exitAtLengthFrac, nextExit))
                {
                    result.emplace_back(obj);
                }
            }

            Vector3D cellCenter = volumeGrid.location(nextCellIdx);

            float timesPerAxis[3];
            float bestTime = leftLength;

            bool reachedEnd = true;
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
                        reachedEnd = false;
                    }
                }
                else
                    timesPerAxis[axis] = FLT_MAX;
            }

            if (reachedEnd) {
                return result.size() > 0;
            }

            for (uint32 axis = 0; axis < 3; axis++)
            {
                if (parallel[axis]) continue;
                nextCellIdx.idx[axis] += (timesPerAxis[axis] <= bestTime + SMALL_EPSILON) ? (dir[axis] > 0) ? 1 : -1 : 0;
                if (nextCellIdx[axis] >= volumeGrid.cellCount()[axis] || nextCellIdx[axis] < 0)
                    return result.size() > 0;
            }

            nextStart += dir * bestTime;
            leftLength -= (dir * bestTime).length();
        }
    }
    return result.size() > 0;
}

template <typename StorageType>
bool BoundingVolume<StorageType>::updateBoundsChecked(const StorageType& object, const AABB& oldBox, const AABB& newBox)
{
    std::set<StorageType> intersections = findIntersection(newBox);
    intersections.erase(object);

    if (intersections.size() > 0)
        return false;

    updateBounds(object, oldBox, newBox);

    return true;
}

template <typename StorageType>
void BoundingVolume<StorageType>::updateBounds(const StorageType& object, const AABB& oldBox, const AABB& newBox)
{
    GridCellIndex minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(oldBox.minBound)));
    GridCellIndex maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(oldBox.maxBound)));

#if USE_CELLIDX_RANGE_ITR
    for (const GridCellIndex& currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
    {
        removeObject(object, currentIdx);
    }
#else // USE_CELLIDX_RANGE_ITR
    GridCellIndex currentIdx;
    for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
    {
        for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
        {
            for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
            {
                removeObject(object, currentIdx);
            }
        }
    }
#endif // USE_CELLIDX_RANGE_ITR

    if(getBounds().intersect(newBox))
    {
        minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(newBox.minBound)));
        maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(newBox.maxBound)));

#if USE_CELLIDX_RANGE_ITR
        for (const GridCellIndex& currentIdx : GridCellIndexRange(minBoundIdx, maxBoundIdx))
        {
            addObject(object, currentIdx);
        }
#else // USE_CELLIDX_RANGE_ITR
        for (currentIdx[2] = minBoundIdx.idx[2]; currentIdx[2] <= maxBoundIdx[2]; currentIdx[2]++)
        {
            for (currentIdx[1] = minBoundIdx.idx[1]; currentIdx[1] <= maxBoundIdx[1]; currentIdx[1]++)
            {
                for (currentIdx[0] = minBoundIdx.idx[0]; currentIdx[0] <= maxBoundIdx[0]; currentIdx[0]++)
                {
                    addObject(object, currentIdx);
                }
            }
        }
#endif // USE_CELLIDX_RANGE_ITR
    }
}

template <typename StorageType>
bool BoundingVolume<StorageType>::isSameBounds(const AABB& boxOne, const AABB& boxTwo)
{
    GridCellIndex minBoundIdx1 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxOne.minBound)));
    GridCellIndex maxBoundIdx1 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxOne.maxBound)));


    GridCellIndex minBoundIdx2 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxTwo.minBound)));
    GridCellIndex maxBoundIdx2 = volumeGrid.clampCellIndex(volumeGrid.cell(volumeGrid.clampLocation(boxTwo.maxBound)));

    return minBoundIdx1 == minBoundIdx2 && maxBoundIdx1 == maxBoundIdx2;
}

#undef USE_CELLIDX_RANGE_ITR