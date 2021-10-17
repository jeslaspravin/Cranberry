#pragma once

#include "CoreMathTypedefs.h"
#include "../Types/Containers/ArrayView.h"
#include "Math.h"
#include "../Platform/PlatformAssertionErrors.h"

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
        : minBound(min)
        , maxBound(max)
    {}

    Box(const T& value)
        : minBound(value)
        , maxBound(value)
    {}

    Box(const ArrayView<T>& points)
        : minBound(0)
        , maxBound(0)
    {
        fatalAssert(points.size() > 0, "Points must have atleast one point");
        offset(points[0]);
        for (uint32 i = 1; i < points.size(); ++i)
        {
            grow(points[i]);
        }
        fixAABB();
    }

    Box(const Box<T, d>& other)
        : minBound(other.minBound)
        , maxBound(other.maxBound)
    {}

    Box(Box<T, d>&& other)
        : minBound(std::move(other.minBound))
        , maxBound(std::move(other.maxBound))
    {}

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
            if (other.maxBound[i] < minBound[i] || other.minBound[i] > maxBound[i])
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
                std::swap(minBound[i], maxBound[i]);
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

    T center() const
    {
        return (maxBound + minBound) * 0.5f;
    }

    void boundCorners(ArrayView<T>& corners) const
    {
        uint32 totalCorners = Math::pow(2, d);
        fatalAssert(corners.size() >= totalCorners, "Corners must be greater or equal to %d", totalCorners);

        const T boundCenter = center();
        const T boundHalfExtend = size() * 0.5f;

        for (uint32 index = 0; index < totalCorners; ++index)
        {
            T offsetMul;
            // finding offsetMul for this corner
            {
                uint32 product = totalCorners;
                uint32 remainder = index;
                // Usually we iterate like for each z { for each y { for each x }}
                for (int32 i = d - 1; i >= 0; --i)
                {
                    product /= 2;
                    int32 gridIdx = remainder / product;
                    remainder -= gridIdx * product;

                    offsetMul[i] = float(gridIdx);
                }
                // 0 - 1 to -0.5 to 0.5 then to -1 to 1
                offsetMul = (offsetMul - T(0.5f)) * T(2);
            }
            corners[index] = boundCenter + boundHalfExtend * offsetMul;
        }
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

template<class T>
class Box<T, 1>
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

    Box(const Box<T, 1>& other)
    {
        minBound = other.minBound;
        maxBound = other.maxBound;
    }

    Box(Box<T, 1>&& other)
    {
        minBound = std::move(other.minBound);
        maxBound = std::move(other.maxBound);
    }

    Box<T, 1>& operator=(const Box<T, 1>& other)
    {
        minBound = other.minBound;
        maxBound = other.maxBound;
        return *this;
    }

    Box<T, 1> operator+(const Box<T, 1>& other)
    {
        Box<T, 1> newBox;
        newBox.minBound = minBound > other.minBound ? other.minBound : minBound;
        newBox.maxBound = maxBound < other.maxBound ? other.maxBound : maxBound;
        return newBox;
    }

    Box<T, 1> operator+(const T& offset)
    {
        Box<T, 1> newBox;
        newBox.minBound = minBound + offset;
        newBox.maxBound = maxBound + offset;
        return newBox;
    }

    void operator+=(const Box<T, 1>& other)
    {
        grow(other);
    }

    void operator+=(const T& dx)
    {
        offset(dx);
    }


    Box<T, 1>& operator=(Box<T, 1>&& other)
    {
        minBound = std::move(other.minBound);
        maxBound = std::move(other.maxBound);
        return *this;
    }

    void offset(const T& offset)
    {
        minBound += offset;
        maxBound += offset;
    }

    void grow(const Box<T, 1>& other)
    {
        minBound = minBound > other.minBound ? other.minBound : minBound;
        maxBound = maxBound < other.maxBound ? other.maxBound : maxBound;
    }
    void grow(const T& point)
    {
        minBound = minBound > point ? point : minBound;
        maxBound = maxBound < point ? point : maxBound;
    }

    bool intersect(const Box<T, 1>& other) const
    {
        return !(other.maxBound < minBound || other.minBound > maxBound);
    }

    Box<T, 1> getIntersectionBox(const Box<T, 1>& other, bool checkAA = true) const
    {
        Box<T, 1> regionBox;
        regionBox.minBound = minBound > other.minBound ? minBound : other.minBound;
        regionBox.maxBound = maxBound < other.maxBound ? maxBound : other.maxBound;

        if (checkAA) 
        {
            regionBox.fixAABB();
        }
        return regionBox;
    }

    bool isValidAABB() const
    {
        return minBound <= maxBound;
    }

    void fixAABB()
    {
        if (minBound > maxBound)
        {
            // Swap in case of swapped axis
            std::swap(minBound, maxBound);
        }
    }

    bool contains(const T& point) const
    {
        return point >= minBound && point <= maxBound;
    }

    T size() const
    {
        return maxBound - minBound;
    }

    T center() const
    {
        return (maxBound + minBound) * 0.5f;
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