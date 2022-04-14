/*!
 * \file VectorND.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Grid.h"

#include <vector>

template <class T, uint32 d>
class VectorND
{

private:
    std::vector<T> data;
    CellIndex<d> cellsCount;

public:
    VectorND() { data.resize(0); }

    explicit VectorND(const CellIndex<d> &count)
    {
        cellsCount = count;
        data.resize(cellsCount.size());
    }

    explicit VectorND(const VectorND<T, d> &other)
    {
        cellsCount = other.cellsCount;
        data = other.data;
    }

    VectorND(VectorND<T, d> &&other)
    {
        data = std::move(other.data);
        cellsCount = std::move(other.cellsCount);
    }

    FORCE_INLINE VectorND &operator=(const VectorND<T, d> &other)
    {
        cellsCount = other.cellsCount;
        data = other.data;
        return *this;
    }

    FORCE_INLINE VectorND &operator=(VectorND<T, d> &&other)
    {
        data = std::move(other.data);
        cellsCount = std::move(other.cellsCount);
        return *this;
    }

    FORCE_INLINE T &operator[](const CellIndex<d> &cell)
    {
        uint32 idx = 0;
        uint32 count = 1;
        // If we are considering x as highest degree such that each x has Y * Z ys and zs
        // Below need iterating for each x { for each y { for each z }}
        // for (int32 i = d - 1; i >= 0; --i)
        //{
        //    idx += count * cell[i];
        //    count *= cellsCount[i];
        //}
        // Usually we iterate like for each z { for each y { for each x }}
        for (int32 i = 0; i < d; ++i)
        {
            idx += count * cell[i];
            count *= cellsCount[i];
        }
        return data[idx];
    }

    FORCE_INLINE void clear()
    {
        cellsCount = CellIndex<d>();
        data.clear();
    }

    FORCE_INLINE void resize(const CellIndex<d> &count)
    {
        cellsCount = count;
        data.resize(cellsCount.size());
    }
};
