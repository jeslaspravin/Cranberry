#pragma once

#include "Grid.h"

#include <vector>

template<class T,uint32 d>
class VectorND 
{

private:
    
    std::vector<T> data;
    CellIndex<d> cellsCount;

public:
    VectorND()
    {
        data.resize(0);
    }

    VectorND(const CellIndex<d>& count)
    {
        cellsCount = count;
        data.resize(cellsCount.size());
    }

    VectorND(const VectorND<T, d>& other)
    {
        cellsCount = other.cellsCount;
        data = other.data;
    }

    VectorND(VectorND<T, d>&& other)
    {
        data = std::move(other.data);
        cellsCount = std::move(other.cellsCount);
    }

    void operator=(const VectorND<T, d>& other)
    {
        cellsCount = other.cellsCount;
        data = other.data;
    }

    void operator=(VectorND<T, d>&& other)
    {
        data = std::move(other.data);
        cellsCount = std::move(other.cellsCount);
    }

    T& operator[](const CellIndex<d>& cell)
    {
        uint32 idx = 0;
        uint32 count = 1;
        for (int32 i = d - 1; i >= 0; --i)
        {
            idx += count * cell[i];
            count *= cellsCount[i];
        }
        return data[idx];
    }

    void clear()
    {
        cellsCount = CellIndex<d>();
        data.clear();
    }

    void resize(const CellIndex<d>& count)
    {
        cellsCount = count;
        data.resize(cellsCount.size());
    }
};
