/*!
 * \file Grid.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Math.h"

template<uint32 d>
struct CellIndex
{
    uint32 idx[d];

    CellIndex() 
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i] = 0;
        }
    }

    explicit CellIndex(uint32 cmnIdx)
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i] = cmnIdx;
        }
    }

    CellIndex(const CellIndex& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i] = other.idx[i];
        }
    }

    CellIndex(CellIndex&& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i] = other.idx[i];
        }
    }

    FORCE_INLINE void operator=(const CellIndex& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i] = other.idx[i];
        }
    }

    template<class T>
    FORCE_INLINE T operator*(const T& other) const
    {
        T newVal;
        for (uint32 i = 0; i < d; i++)
        {
            newVal[i] = other[i] * idx[i];
        }
        return newVal;
    }

    FORCE_INLINE CellIndex operator+(const CellIndex& other) const
    {
        CellIndex c;
        for (uint32 i = 0; i < d; i++)
        {
            c.idx[i] = idx[i] + other.idx[i];
        }

        return c;
    }


    FORCE_INLINE CellIndex operator-(const CellIndex& other) const
    {
        CellIndex c;
        for (uint32 i = 0; i < d; i++)
        {
            c.idx[i] = idx[i] < other.idx[i]
                ? other.idx[i] - idx[i] : idx[i] - other.idx[i];
        }

        return c;
    }


    FORCE_INLINE void operator+=(const CellIndex& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
             idx[i] += other.idx[i];
        }
    }

    FORCE_INLINE void operator-=(const CellIndex& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i] = idx[i] < other.idx[i]
                ? other.idx[i] - idx[i] : idx[i] - other.idx[i];
        }
    }

    FORCE_INLINE uint32 operator[](const uint32& axis) const
    {
        if (axis < d)
        {
            return idx[axis];
        }
        return -1;
    }

    FORCE_INLINE uint32& operator[](const uint32& axis)
    {
        return idx[axis];
    }

    FORCE_INLINE bool operator==(const CellIndex& other) const
    {
        bool bIsSame = true;
        for (uint32 i = 0; i < d; i++)
        {
            bIsSame = bIsSame && (idx[i] == other.idx[i]);
        }
        return bIsSame;
    }

    FORCE_INLINE bool operator!=(const CellIndex& other) const
    {
        bool bIsNotSame = false;
        for (uint32 i = 0; i < d; i++)
        {
            bIsNotSame = bIsNotSame || (idx[i] != other.idx[i]);
        }
        return bIsNotSame;
    }

    FORCE_INLINE uint32 size() const
    {
        uint32 s = 1;
        for (uint32 i = 0; i < d; i++)
        {
            s *= idx[i];
        }
        return s;
    }
};

template<uint32 d>
struct CellIndexHash
{
    using TYPE = CellIndex<d>;
    using RESULT = std::size_t;

    RESULT operator()(const TYPE& cellIndex) const 
    {
        RESULT hashVal = 0;
        for (uint32 i = 0; i < d; i++)
        {
            hashCombine(hashVal, cellIndex[i]);
        }
        return hashVal;
    }
};

template<uint32 d>
class CellIndexRangeIterator
{
private:
    CellIndex<d> lower;
    CellIndex<d> higher;
    CellIndex<d> currentIdx;

    CellIndexRangeIterator() = default;

    // Ensure that min is less than max in all dimension
    CellIndexRangeIterator(const CellIndex<d>& minRange, const CellIndex<d>& maxRange)
        : lower(minRange)
        // Since we have to do <= with max range for cells
        // , we make each dimension + 1. So when values reaches end()(which will be value 1 over higher) it ends
        , higher(maxRange + CellIndex<d>(1))
        , currentIdx(minRange)
    {
        for (uint32 dim = 0; dim < d; ++dim)
        {
            debugAssert(lower[dim] <= higher[dim]);
        }
    }
public:
    CellIndexRangeIterator(const CellIndexRangeIterator& itr)
        : lower(itr.lower)
        , higher(itr.higher)
        , currentIdx(itr.currentIdx)
    {}
    CellIndexRangeIterator(CellIndexRangeIterator&& itr)
        : lower(std::move(itr.lower))
        , higher(std::move(itr.higher))
        , currentIdx(std::move(itr.currentIdx))
    {}

    FORCE_INLINE const CellIndex<d>& operator->() const
    {
        return currentIdx;
    }

    FORCE_INLINE const CellIndex<d>& operator*() const
    {
        return currentIdx;
    }

    FORCE_INLINE bool operator!=(const CellIndexRangeIterator& other) const
    {
        return currentIdx != other.currentIdx;
    }

    FORCE_INLINE CellIndexRangeIterator& operator++()
    {
        ++currentIdx[0];
        for (uint32 dim = 1; dim < d; ++dim)
        {
            currentIdx[dim] += (currentIdx[dim - 1] / higher[dim - 1]);
            currentIdx[dim - 1] = Math::max(currentIdx[dim - 1] % higher[dim - 1], lower[dim - 1]);
        }
        return *this;
    }

    FORCE_INLINE CellIndexRangeIterator operator++(int)
    {
        CellIndexRangeIterator retVal(*this);
        this->operator++();
        return retVal;
    }

    static CellIndexRangeIterator beginRange(const CellIndex<d>& minRange, const CellIndex<d>& maxRange)
    {
        return CellIndexRangeIterator(minRange, maxRange);
    }

    static CellIndexRangeIterator endRange(const CellIndex<d>& minRange, const CellIndex<d>& maxRange)
    {
        CellIndexRangeIterator retVal;
        // Since we have to do <= with max range for cells
        // , we make each dimension + 1. So when values reaches end()(which will be value 1 over higher) it ends
        retVal.currentIdx = minRange;
        retVal.currentIdx[d - 1] = maxRange[d - 1] + 1;
        return retVal;
    }
};

template<uint32 d>
struct CellIndexRange
{
    using IteratorType = CellIndexRangeIterator<d>;
    IteratorType beginItr;
    IteratorType endItr;

    CellIndexRange(const CellIndex<d>& minRange, const CellIndex<d>& maxRange)
        : beginItr(IteratorType::beginRange(minRange, maxRange))
        , endItr(IteratorType::endRange(minRange, maxRange))
    {}

    IteratorType begin() const
    {
        return beginItr;
    }

    IteratorType end() const
    {
        return endItr;
    }
};

template <class T,uint32 d>
FORCE_INLINE void vectorToCellIdx(T vec, CellIndex<d>& cellIdx)
{
    for (uint32 i = 0; i < d; i++)
    {
        cellIdx.idx[i] = uint32(vec[i]);
    }
}

template <class T, uint32 d>
FORCE_INLINE CellIndex<d> vectorToCellIdx(T vec)
{
    CellIndex<d> cellIdx;
    for (uint32 i = 0; i < d; i++)
    {
        cellIdx.idx[i] = uint32(vec[i]);
    }
    return cellIdx;
}

template<class T,uint32 d>
class UniformGrid 
{

private:

    T nCells;
    T cellDx;
    T minCorner;
    T maxCorner;

public:

    UniformGrid()
        : nCells(0)
        , cellDx(0)
        , minCorner(0)
        , maxCorner(0)
    {}

    DEBUG_INLINE void InitWithCount(const T& min, const T& max, const CellIndex<d>& n);
    DEBUG_INLINE void InitWithSize(const T& min, const T& max, const T& cellSize);

    FORCE_INLINE T location(const CellIndex<d>& cell) const
    {
        return cell*cellDx + minCorner + (cellDx * 0.5f);
    }

    // Location must be greater than minimum bound to get valid cell index(as -ve values cannot be stored in unsigned grid cell idx)
    FORCE_INLINE CellIndex<d> cell(const T& location) const
    {
        return vectorToCellIdx<T, d>(Math::floor((location - minCorner) / cellDx));
    }

    FORCE_INLINE CellIndex<d> getNdIndex(const uint32 index) const;

    FORCE_INLINE T center(const uint32 index) const
    {
        return location(getNdIndex(index));
    }

    FORCE_INLINE CellIndex<d> cellCount() const
    {
        return vectorToCellIdx<T, d>(nCells);
    }

    FORCE_INLINE void cellCount(CellIndex<d>& cellCnt) const
    {
        vectorToCellIdx<T,d>(nCells, cellCnt);
    }

    FORCE_INLINE CellIndex<d> clampCellIndex(const CellIndex<d>& cell) const;
    FORCE_INLINE T clampLocation(const T& location) const;
    FORCE_INLINE bool isInside(const T& location) const;
    FORCE_INLINE bool isInside(const CellIndex<d> cell) const;

    FORCE_INLINE T cellSize() const
    {
        return cellDx;
    }

    FORCE_INLINE void getBound(T& minB, T& maxB) const
    {
        minB = minCorner;
        maxB = maxCorner;
    }
};

template<class T, uint32 d>
FORCE_INLINE CellIndex<d> UniformGrid<T, d>::getNdIndex(const uint32 index) const
{
    CellIndex<d> ndIndex;
    uint32 product = 1, remainder = index;
    for (uint32 i = 0; i < d; ++i)
    {
        product *= (uint32)nCells[i];
    }
    // Usually we iterate like for each z { for each y { for each x }}
    for (int32 i = d - 1; i >= 0; --i)
    {
        product /= (uint32)nCells[i];
        ndIndex.idx[i] = remainder / product;
        remainder -= ndIndex[i] * product;
    }
    return ndIndex;
}

template<class T, uint32 d>
FORCE_INLINE CellIndex<d> UniformGrid<T, d>::clampCellIndex(const CellIndex<d>& cell) const
{
    CellIndex<d> clampedCell;
    for (uint32 i = 0; i < d; i++)
    {
        clampedCell.idx[i] = Math::clamp(cell[i], 0u, uint32(nCells[i] - 1));
    }
    return clampedCell;
}

template<class T, uint32 d>
FORCE_INLINE T UniformGrid<T, d>::clampLocation(const T& location) const
{
    T clampedLocation;
    for (uint32 i = 0; i < d; i++) 
    {
        clampedLocation[i] = Math::clamp(location[i], minCorner[i], maxCorner[i]);
    }
    return clampedLocation;
}

template<class T, uint32 d>
FORCE_INLINE bool UniformGrid<T, d>::isInside(const T& location) const
{
    for (uint32 i = 0; i < d; i++)
    {
        if (location[i] < minCorner[i] || location[i] > maxCorner[i])
        {
            return false;
        }
    }
    return true;
}

template<class T, uint32 d>
FORCE_INLINE bool UniformGrid<T, d>::isInside(const CellIndex<d> cell) const
{
    for (uint32 i = 0; i < d; i++) 
    {
        if (cell[i] >= (uint32)nCells[i] || cell[i] < 0)
        {
            return false;
        }
    }
    return true;
}

template<class T, uint32 d>
DEBUG_INLINE void UniformGrid<T, d>::InitWithSize(const T& min, const T& max, const T& cellSize)
{
    minCorner = min;
    maxCorner = max;
    cellDx = cellSize;
    // Validate min max corners
    for (uint32 i = 0; i < d; i++)
    {
        if (minCorner[i] > maxCorner[i])
        {
            std::swap(maxCorner[i], minCorner[i]);
        }
    }

    T temp = maxCorner - minCorner;
    temp = temp / cellDx;
    nCells = Math::floor(temp);
    temp = temp - nCells;

    // Always have 1 extra border
    nCells += T(1);
    maxCorner = minCorner + nCells * cellDx;
}

template<class T, uint32 d>
DEBUG_INLINE void UniformGrid<T, d>::InitWithCount(const T& min, const T& max, const CellIndex<d>& n)
{
    minCorner = min;
    maxCorner = max;

    for (uint32 i = 0; i < d; i++)
    {
        if (minCorner[i] > maxCorner[i])
        {
            std::swap(maxCorner[i], minCorner[i]);
        }
        nCells[i] = (float)n[i];
    }

    T diff = maxCorner - minCorner;
    cellDx = diff / nCells;
}

