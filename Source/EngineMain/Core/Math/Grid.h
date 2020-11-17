#pragma once

#include "Math.h"

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

    CellIndex(uint32 cmnIdx) 
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

    void operator=(const CellIndex& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i] = other.idx[i];
        }
    }

    template<class T>
    T operator*(const T& other) const
    {
        T newVal;
        for (uint32 i = 0; i < d; i++)
        {
            newVal[i] = other[i] * idx[i];
        }
        return newVal;
    }

    CellIndex operator+(const CellIndex& other)
    {
        CellIndex c;
        for (uint32 i = 0; i < d; i++)
        {
            c.idx[i] = idx[i] + other.idx[i];
        }

        return c;
    }


    CellIndex operator-(const CellIndex& other)
    {
        CellIndex c;
        for (uint32 i = 0; i < d; i++)
        {
            c.idx[i] = Math::abs(idx[i] - other.idx[i]);
        }

        return c;
    }


    void operator+=(const CellIndex& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
             idx[i] += other.idx[i];
        }
    }

    void operator-=(const CellIndex& other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            idx[i]=glm::abs(idx[i] - other.idx[i]);
        }
    }

    uint32 operator[](const uint32& axis) const
    {
        if (axis < d)
        {
            return idx[axis];
        }
        return -1;
    }

    bool operator==(const CellIndex& other) const
    {
        for (uint32 i = 0; i < d; i++)
        {
            if (idx[i] != other.idx[i])
                return false;
        }
        return true;
    }

    uint32 size() {
        uint32 s = 1;
        for (uint32 i = 0; i < d; i++)
        {
            s *= idx[i];
        }
        return s;
    }
};

template<uint32 d>
struct CellIndexHash{
    using TYPE = CellIndex<d>;
    using RESULT = std::size_t;

    RESULT operator ()(const TYPE& cellIndex) const {
        RESULT hashVal = 0;
        for (uint32 i = 0; i < d; i++)
        {
            hashCombine(hashVal, cellIndex[i]);
        }
        return hashVal;
    }
};

template <class T,uint32 d>
void vectorToCellIdx(T vec, CellIndex<d>& cellIdx)
{
    for (uint32 i = 0; i < d; i++)
    {
        cellIdx.idx[i] = (uint32)vec[i];
    }
}

template <class T, uint32 d>
CellIndex<d> vectorToCellIdx(T vec)
{
    CellIndex<d> cellIdx;
    for (uint32 i = 0; i < d; i++)
    {
        cellIdx.idx[i] = (uint32)vec[i];
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

    void InitWithCount(const T& min, const T& max, const CellIndex<d>& n);
    void InitWithSize(const T& min, const T& max, const T& cellSize);

    T location(const CellIndex<d>& cell) const
    {
        return cell*cellDx + minCorner + (cellDx * 0.5f);
    }

    CellIndex<d> cell(const T& location) const
    {
        return vectorToCellIdx<T, d>((location - minCorner) / cellDx);
    }

    CellIndex<d> getNdIndex(const uint32 index) const;

    T center(const uint32 index) const
    {
        return location(getNdIndex(index));
    }

    CellIndex<d> cellCount()
    {
        return vectorToCellIdx<T, d>(nCells);
    }

    void cellCount(CellIndex<d>& cellCnt)
    {
        vectorToCellIdx<T,d>(nCells, cellCnt);
    }

    CellIndex<d> clampCellIndex(const CellIndex<d>& cell);
    T clampLocation(const T& location);
    bool isInside(const CellIndex<d> cell);

    T cellSize() {
        return cellDx;
    }

    void getBound(T& minB, T& maxB)
    {
        minB = minCorner;
        maxB = maxCorner;
    }
};

template<class T, uint32 d>
CellIndex<d> UniformGrid<T, d>::getNdIndex(const uint32 index) const
{
    CellIndex<d> ndIndex;
    uint32 product = 1, remainder = index;
    for (uint32 i = 0; i < d; ++i)
    {
        product *= (uint32)nCells[i];
    }
    for (uint32 i = 0; i < d; ++i)
    {
        product /= (uint32)nCells[i];
        ndIndex.idx[i] = remainder / product;
        remainder -= ndIndex[i] * product;
    }
    return ndIndex;
}

template<class T, uint32 d>
CellIndex<d> UniformGrid<T, d>::clampCellIndex(const CellIndex<d>& cell)
{
    CellIndex<d> clampedCell;
    for (uint32 i = 0; i < d; i++) {
        clampedCell.idx[i] = Math::clamp(cell[i], 0u, (uint32)nCells[i]);
    }
    return clampedCell;
}

template<class T, uint32 d>
T UniformGrid<T, d>::clampLocation(const T& location)
{
    T clampedLocation;
    for (uint32 i = 0; i < d; i++) {
        clampedLocation[i] = Math::clamp(location[i], minCorner[i], maxCorner[i]);
    }
    return clampedLocation;
}

template<class T, uint32 d>
bool UniformGrid<T, d>::isInside(const CellIndex<d> cell)
{
    for (uint32 i = 0; i < d; i++) {
        if (cell[i] >= (uint32)nCells[i] || cell[i] < 0)
        {
            return false;
        }
    }
    return true;
}

template<class T, uint32 d>
void UniformGrid<T, d>::InitWithSize(const T& min, const T& max, const T& cellSize)
{
    minCorner = min;
    maxCorner = max;
    cellDx = cellSize;

    T temp = maxCorner - minCorner;
    temp = temp / cellDx;
    nCells = Math::floor(temp);
    temp = temp - nCells;

    for (uint32 i = 0; i < d; i++)
    {
        if (temp[i] > 0) {
            nCells += T(1);
            maxCorner = minCorner + nCells * cellDx;
            break;
        }
    }
}

template<class T, uint32 d>
void UniformGrid<T, d>::InitWithCount(const T& min, const T& max, const CellIndex<d>& n)
{
    minCorner = min;
    maxCorner = max;

    for (uint32 i = 0; i < d; i++) {
        nCells[i] = (float)n[i];
    }

    T diff = maxCorner - minCorner;
    cellDx = diff / nCells;
}

