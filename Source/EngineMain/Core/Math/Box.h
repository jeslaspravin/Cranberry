#pragma once

#include "CoreMathTypedefs.h"
#include "Math.h"

class Vector2D;
class Vector3D;

// Usage AABB
template<class T,uint32 d>
class Box
{
public:
    T minBound;
    T maxBound;

    Box() = default;

    Box(const T& min, const T& max)
    {
        minBound = min;
        maxBound = max;
    }

    Box(const Box<T, d>& other)
    {
        minBound = other.minBound;
        maxBound = other.maxBound;
    }

    Box(Box<T, d>&& other)
    {
        minBound = std::move(other.minBound);
        maxBound = std::move(other.maxBound);
    }

    Box<T, d>& operator=(const Box<T, d>& other)
    {
        minBound = other.minBound;
        maxBound = other.maxBound;
        return *this;
    }

    Box<T, d> operator+(const Box<T, d>& other)
    {
        Box<T, d> newBox;
        for (uint32 i = 0; i < d; i++)
        {
            newBox.minBound[i] = minBound[i] > other.minBound[i] ? other.minBound[i] : minBound[i];
            newBox.maxBound[i] = maxBound[i] < other.maxBound[i] ? other.maxBound[i] : maxBound[i];
        }
        return newBox;
    }

    Box<T, d> operator+(const T& offset)
    {
        Box<T, d> newBox;
        for (uint32 i = 0; i < d; i++)
        {
            newBox.minBound[i] = minBound[i] + offset[i];
            newBox.maxBound[i] = maxBound[i] + offset[i];
        }
        return newBox;
    }

    void operator+=(const Box<T, d>& other)
    {
        grow(other);
    }

    void operator+=(const T& dx)
    {
        offset(dx);
    }


    Box<T, d>& operator=(Box<T, d>&& other)
    {
        minBound = std::move(other.minBound);
        maxBound = std::move(other.maxBound);
        return *this;
    }

    void offset(const T& offset)
    {
        for (uint32 i = 0; i < d; i++)
        {
            minBound[i] += offset[i];
            maxBound[i] += offset[i];
        }
    }

    void grow(const Box<T, d>& other) 
    {
        for (uint32 i = 0; i < d; i++)
        {
            minBound[i] = minBound[i] > other.minBound[i] ? other.minBound[i] : minBound[i];
            maxBound[i] = maxBound[i] < other.maxBound[i] ? other.maxBound[i] : maxBound[i];
        }
    }
    void grow(const T& point)
    {
        for (uint32 i = 0; i < d; i++)
        {
            minBound[i] = minBound[i] > point[i] ? point[i] : minBound[i];
            maxBound[i] = maxBound[i] < point[i] ? point[i] : maxBound[i];
        }
    }
    
    bool intersect(const Box<T, d>& other) const 
    {
        for (uint32 i = 0; i < d; i++)
        {
            // If min point of one is larger than max point of another or vice versa then box never intersects
            if (other.maxBound[i]<minBound[i] || other.minBound[i] > maxBound[i])
                return false;
        }
        return true;
    }

    Box<T, d> getIntersectionBox(const Box<T, d>& other,bool checkAA=true) const
    {
        Box<T, d> regionBox;
        for (uint32 i = 0; i < d; i++)
        {
            regionBox.minBound[i] = minBound[i] > other.minBound[i] ? minBound[i] : other.minBound[i];
            regionBox.maxBound[i] = maxBound[i] < other.maxBound[i] ? maxBound[i] : other.maxBound[i];
        }

        if (checkAA) {
            regionBox.fixAABB();
        }
        return regionBox;
    }

    bool isValidAABB() const
    {
        for (uint32 i = 0; i < d; i++)
        {
            if (minBound[i] > maxBound[i])
            {
                return false;
            }
        }
        return true;
    }

    void fixAABB()
    {
        for (uint32 i = 0; i < d; i++)
        {
            if (minBound[i] > maxBound[i])
            {
                // Swap in case of swapped axis
                minBound[i] = (minBound[i] + maxBound[i]) - (maxBound[i] = minBound[i]);
            }
        }
    }

    bool contains(const T& point) const
    {
        for (uint32 i = 0; i < d; i++)
        {
            if (point[i] < minBound[i] || point[i] > maxBound[i])
            {
                return false;
            }
        }
        return true;
    }

    T size() const
    {
        return maxBound - minBound;
    }

    // Ensure start point is outside the box
    // Out Lengths are portion of ray segment to reach the point
    bool raycast(const T& startPoint, const T& dir, const float& length, float& invLength, float& outEnterLength,
        T& outEnterPoint, float& outExitLength, T& outExitPoint) const
    {
        bool parallel[d];
        T invDir;

        for (uint32 i = 0; i < d; i++)
        {
            parallel[i] = (dir[i] == 0);
            invDir[i] = parallel[i] ? 0 : 1/dir[i];
        }

        return raycastFast(startPoint, dir, invDir, length, invLength, &parallel[0], outEnterLength, outEnterPoint, outExitLength, outExitPoint);
    }

    bool raycastFast(const T& startPoint, const T& dir,const T& invDir, const float& length, float& invLength,const bool* const parallel,
        float& outEnterLength,T& outEnterPoint, float& outExitLength, T& outExitPoint) const
    {
        T s2Min = minBound - startPoint;
        T s2Max = maxBound - startPoint;

        float enteringTime = 0;
        float exitTime = FLT_MAX;

        for (uint32 axis = 0; axis < d; axis++)
        {
            float t1, t2;
            if (parallel[axis])
            {
                if (s2Min[axis] > 0 || s2Max[axis] < 0)
                {
                    return false;
                }
                else {
                    t2 = FLT_MAX;
                    t1 = 0;
                }
            }
            else {
                t1 = s2Min[axis] * invDir[axis];
                t2 = s2Max[axis] * invDir[axis];
            }

            if (t2 < t1)
            {
                float t = t2;
                t2 = t1;
                t1 = t;
            }

            enteringTime = Math::max(t1, enteringTime);
            exitTime = Math::min(t2, exitTime);

            if (exitTime < enteringTime)
                return false;
        }

        if (enteringTime > length || exitTime < 0)
            return false;

        outEnterLength = enteringTime * invLength;
        outEnterPoint = startPoint + dir * enteringTime;
        outExitLength = exitTime * invLength;
        outExitPoint = startPoint + dir * exitTime;

        return true;
    }
};

template<typename T>
using ValueRange = Box<T, 1>;

using SizeBox2D = Box<Size2D, 2>;
using SizeBox3D = Box<Size3D, 3>;

using QuantizedBox2D = Box<Int2D, 2>;
using QuantizedBox3D = Box<Int3D, 3>;

using Rect = Box<Vector2D, 2>;
using AABB = Box<Vector3D, 3>;