#pragma once

#include "../Platform/PlatformAssertionErrors.h"
#include "Grid.h"
#include "Box.h"
#include "VectorND.h"

#include <list>

template <typename StorageType>
class BoundingVolume 
{
private:
    UniformGrid<Vector3D, 3> volumeGrid;
    VectorND<std::list<StorageType>, 3> grid;

    std::list<StorageType> allObjects;

    bool isWithinBounds(const CellIndex<3>& minBoundIdx, const CellIndex<3>& maxBoundIdx, const AABB& box) const;
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
    IntersectionSet findIntersection(const AABB& box, bool bSkipObjChecks)
    {
        IntersectionSet intersection;

        CellIndex<3> minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(box.minBound));
        CellIndex<3> maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(box.maxBound));

        if (!isWithinBounds(minBoundIdx, maxBoundIdx, box))
        {
            return intersection;
        }

        CellIndex<3> currentIdx;
        for (uint32 x = minBoundIdx.idx[0]; x <= maxBoundIdx[0]; x++)
        {
            for (uint32 y = minBoundIdx.idx[1]; y <= maxBoundIdx[1]; y++)
            {
                for (uint32 z = minBoundIdx.idx[2]; z <= maxBoundIdx[2]; z++)
                {
                    vectorToCellIdx(Size3D(x, y, z), currentIdx);
                    const std::list<StorageType>& objs = grid[currentIdx];
                    if (bSkipObjChecks)
                    {
                        intersection.insert(objs.cbegin(), objs.cend());
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
        return intersection;
    }

    bool raycast(const Vector3D& start, const Vector3D& dir, const float& length, std::vector<StorageType>& result, bool bExistOnHit = true);

    bool updateBoundsChecked(const StorageType& object, const AABB& oldBox, const AABB& newBox);
    void updateBounds(const StorageType& object, const AABB& oldBox, const AABB& newBox);
    bool isSameBounds(const AABB& boxOne, const AABB& boxTwo);
    AABB getBounds() const;
};

template <typename StorageType>
bool BoundingVolume<StorageType>::isWithinBounds(const CellIndex<3>& minBoundIdx, const CellIndex<3>& maxBoundIdx, const AABB& box) const
{
    // if any dimension is clamped because of being outside the volume bound
    //bool bIsAnyDimensionClamped = false;
    //Vector3D boxExtend = box.size();
    //for (int32 i = 0; i < 3; ++i)
    //{
    //    uint32 cellExtend = maxBoundIdx[i] - minBoundIdx[i];
    //    // Invalid cell extend in volume bound while box is not thin plane
    //    bIsAnyDimensionClamped = bIsAnyDimensionClamped || (cellExtend == 0 && !Math::isEqual(boxExtend[i], 0.0f));
    //}
    //return !bIsAnyDimensionClamped;

    return getBounds().intersect(box);
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

    Logger::debug("BVH", "%s() : Before correcting cell size global bounding box size is (%f, %f, %f)", __func__, globalBound.size().x(), globalBound.size().y(), globalBound.size().z());

    volumeGrid.InitWithSize(globalBound.minBound, globalBound.maxBound, cellSize);
    volumeGrid.getBound(globalBound.minBound, globalBound.maxBound);

    Logger::debug("BVH", "%s() : After correcting cell size global bounding box size is (%f, %f, %f)", __func__, globalBound.size().x(), globalBound.size().y(), globalBound.size().z());

    CellIndex<3> cellCount = volumeGrid.cellCount();
    grid = VectorND<std::list<StorageType>, 3>(cellCount);


    for (const StorageType& obj : allObjects)
    {
        const AABB& bound = obj.getBounds();
        CellIndex<3> minBoundIdx = volumeGrid.cell(bound.minBound);
        CellIndex<3> maxBoundIdx = volumeGrid.cell(bound.maxBound);

        CellIndex<3> currentIdx;
        for (uint32 x = minBoundIdx.idx[0]; x <= maxBoundIdx[0]; x++)
        {
            for (uint32 y = minBoundIdx.idx[1]; y <= maxBoundIdx[1]; y++)
            {
                for (uint32 z = minBoundIdx.idx[2]; z <= maxBoundIdx[2]; z++)
                {
                    // Logger::debug("BVH", "%s() : Adding %d to (%d,%d,%d)", __func__, obj, x, y, z);
                    vectorToCellIdx(Size3D(x, y, z), currentIdx);
                    grid[currentIdx].push_back(obj);
                }
            }
        }
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
        CellIndex<3> newCellCount = volumeGrid.cellCount();
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
                CellIndex<3> gridIndex = newGrid.cell(volumeGrid.center(i));
                newElements[gridIndex] = grid[volumeGrid.getNdIndex(i)];
            }
            volumeGrid.InitWithCount(newMinCorner, newMaxCorner, newCellCount);
            grid = std::move(newElements);
        }
    }

    CellIndex<3> minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(objBound.minBound));
    CellIndex<3> maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(objBound.maxBound));

    CellIndex<3> currentIdx;
    for (uint32 x = minBoundIdx.idx[0]; x <= maxBoundIdx[0]; x++)
    {
        for (uint32 y = minBoundIdx.idx[1]; y <= maxBoundIdx[1]; y++)
        {
            for (uint32 z = minBoundIdx.idx[2]; z <= maxBoundIdx[2]; z++)
            {
                vectorToCellIdx(Size3D(x, y, z), currentIdx);
                grid[currentIdx].push_back(object);
            }
        }
    }

    allObjects.push_back(object);
}

template <typename StorageType>
void BoundingVolume<StorageType>::removeAnObject(const StorageType& object)
{
    const AABB& objBound = object.getBounds();

    CellIndex<3> minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(objBound.minBound));
    CellIndex<3> maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(objBound.maxBound));

    CellIndex<3> currentIdx;
    for (uint32 x = minBoundIdx.idx[0]; x <= maxBoundIdx[0]; x++)
    {
        for (uint32 y = minBoundIdx.idx[1]; y <= maxBoundIdx[1]; y++)
        {
            for (uint32 z = minBoundIdx.idx[2]; z <= maxBoundIdx[2]; z++)
            {
                vectorToCellIdx(Size3D(x, y, z), currentIdx);
                grid[currentIdx].remove(object);
            }
        }
    }

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

    if (cellsInPath) {

        Vector3D halfCellSize = volumeGrid.cellSize() * 0.5f;
        CellIndex<3> nextCellIdx = volumeGrid.cell(nextStart);
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
    std::vector<StorageType> intersections = findIntersection(newBox);
    auto newEnd = std::remove_if(intersections.begin(), intersections.end(), [&object](const StorageType& otherEntity)
        {
            return object == otherEntity;
        });
    intersections.erase(newEnd, intersections.end());

    if (intersections.size() > 0)
        return false;

    updateBounds(object, oldBox, newBox);

    return true;
}

template <typename StorageType>
void BoundingVolume<StorageType>::updateBounds(const StorageType& object, const AABB& oldBox, const AABB& newBox)
{
    CellIndex<3> minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(oldBox.minBound));
    CellIndex<3> maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(oldBox.maxBound));

    CellIndex<3> currentIdx;
    for (uint32 x = minBoundIdx.idx[0]; x <= maxBoundIdx[0]; x++)
    {
        for (uint32 y = minBoundIdx.idx[1]; y <= maxBoundIdx[1]; y++)
        {
            for (uint32 z = minBoundIdx.idx[2]; z <= maxBoundIdx[2]; z++)
            {
                vectorToCellIdx(Size3D(x, y, z), currentIdx);
                std::list<StorageType>& objectIndices = grid[currentIdx];
                objectIndices.remove(object);
            }
        }
    }

    minBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(newBox.minBound));
    maxBoundIdx = volumeGrid.clampCellIndex(volumeGrid.cell(newBox.maxBound));

    if (isWithinBounds(minBoundIdx, maxBoundIdx, newBox))
    {
        for (uint32 x = minBoundIdx.idx[0]; x <= maxBoundIdx[0]; x++)
        {
            for (uint32 y = minBoundIdx.idx[1]; y <= maxBoundIdx[1]; y++)
            {
                for (uint32 z = minBoundIdx.idx[2]; z <= maxBoundIdx[2]; z++)
                {
                    vectorToCellIdx(Size3D(x, y, z), currentIdx);
                    std::list<StorageType>& objectIndices = grid[currentIdx];
                    objectIndices.push_back(object);
                }
            }
        }
    }
}

template <typename StorageType>
bool BoundingVolume<StorageType>::isSameBounds(const AABB& boxOne, const AABB& boxTwo)
{
    CellIndex<3> minBoundIdx1 = volumeGrid.clampCellIndex(volumeGrid.cell(boxOne.minBound));
    CellIndex<3> maxBoundIdx1 = volumeGrid.clampCellIndex(volumeGrid.cell(boxOne.maxBound));


    CellIndex<3> minBoundIdx2 = volumeGrid.clampCellIndex(volumeGrid.cell(boxTwo.minBound));
    CellIndex<3> maxBoundIdx2 = volumeGrid.clampCellIndex(volumeGrid.cell(boxTwo.maxBound));

    return minBoundIdx1 == minBoundIdx2 && maxBoundIdx1 == maxBoundIdx2;
}
