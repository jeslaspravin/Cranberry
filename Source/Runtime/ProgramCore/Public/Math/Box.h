/*!
 * \file Box.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypedefs.h"
#include "Math/Math.h"
#include "Types/Containers/ArrayView.h"
#include "Types/Platform/PlatformAssertionErrors.h"

class Vector2D;
class Vector3D;

// Usage AABB
template <class T, uint32 d>
class Box
{
public:
    using PointType = T;
    using PointElementType = typename PointType::value_type;
    CONST_EXPR static const uint32 Dim = d;

    using value_type = PointType;

public:
    T minBound;
    T maxBound;

    Box() = default;

    Box(const T &min, const T &max)
        : minBound(min)
        , maxBound(max)
    {}

    explicit Box(const T &value)
        : minBound(value)
        , maxBound(value)
    {}

    Box(const ArrayView<T> &points)
        : minBound(0)
        , maxBound(0)
    {
        fatalAssertf(points.size() > 0, "Points must have atleast one point");
        offset(points[0]);
        for (uint32 i = 1; i < points.size(); ++i)
        {
            grow(points[i]);
        }
        fixAABB();
    }

    Box(const Box<T, d> &other)
        : minBound(other.minBound)
        , maxBound(other.maxBound)
    {}

    Box(Box<T, d> &&other)
        : minBound(std::move(other.minBound))
        , maxBound(std::move(other.maxBound))
    {}

    Box<T, d> &operator=(const Box<T, d> &other)
    {
        minBound = other.minBound;
        maxBound = other.maxBound;
        return *this;
    }

    Box<T, d> operator+(const Box<T, d> &other)
    {
        Box<T, d> newBox;
        for (uint32 i = 0; i < d; i++)
        {
            newBox.minBound[i] = minBound[i] > other.minBound[i] ? other.minBound[i] : minBound[i];
            newBox.maxBound[i] = maxBound[i] < other.maxBound[i] ? other.maxBound[i] : maxBound[i];
        }
        return newBox;
    }

    Box<T, d> operator+(const T &offset)
    {
        Box<T, d> newBox;
        for (uint32 i = 0; i < d; i++)
        {
            newBox.minBound[i] = minBound[i] + offset[i];
            newBox.maxBound[i] = maxBound[i] + offset[i];
        }
        return newBox;
    }

    void operator+=(const Box<T, d> &other) { grow(other); }

    void operator+=(const T &dx) { offset(dx); }

    Box<T, d> &operator=(Box<T, d> &&other)
    {
        minBound = std::move(other.minBound);
        maxBound = std::move(other.maxBound);
        return *this;
    }

    void reset(const T &min, const T &max)
    {
        minBound = min;
        maxBound = max;
    }

    void offset(const T &offset)
    {
        for (uint32 i = 0; i < d; i++)
        {
            minBound[i] += offset[i];
            maxBound[i] += offset[i];
        }
    }

    void grow(const Box<T, d> &other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            minBound[i] = minBound[i] > other.minBound[i] ? other.minBound[i] : minBound[i];
            maxBound[i] = maxBound[i] < other.maxBound[i] ? other.maxBound[i] : maxBound[i];
        }
    }
    void grow(const T &point)
    {
        for (uint32 i = 0; i < d; i++)
        {
            minBound[i] = minBound[i] > point[i] ? point[i] : minBound[i];
            maxBound[i] = maxBound[i] < point[i] ? point[i] : maxBound[i];
        }
    }

    bool intersect(const Box<T, d> &other) const
    {
        for (uint32 i = 0; i < d; i++)
        {
            // If min point of one is larger than max point of another or vice versa then box never
            // intersects
            if (other.maxBound[i] < minBound[i] || other.minBound[i] > maxBound[i])
                return false;
        }
        return true;
    }

    Box<T, d> getIntersectionBox(const Box<T, d> &other, bool checkAA = true) const
    {
        Box<T, d> regionBox;
        for (uint32 i = 0; i < d; i++)
        {
            regionBox.minBound[i] = minBound[i] > other.minBound[i] ? minBound[i] : other.minBound[i];
            regionBox.maxBound[i] = maxBound[i] < other.maxBound[i] ? maxBound[i] : other.maxBound[i];
        }

        if (checkAA)
        {
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

    bool contains(const T &point) const
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
    bool contains(const Box<T, d> &other)
    {
        for (uint32 i = 0; i < d; i++)
        {
            // If any min bound is less than this or if max bound is greater than this it means that
            // box cannot be contained in this box
            if (other.minBound[i] < minBound[i] || other.maxBound[i] > maxBound[i])
            {
                return false;
            }
        }
        return true;
    }

    // Returns 0 if this cannot contain, 1 if is within this, 2 if other is exactly on the border or is
    // matching the box exactly
    uint8 encloses(const T &point) const
    {
        bool bIsOnBorder = false;
        for (uint32 i = 0; i < d; i++)
        {
            if (point[i] < minBound[i] || point[i] > maxBound[i])
            {
                return 0;
            }
            // If even one axis in on border, point is in border
            bIsOnBorder = bIsOnBorder || Math::isEqual(point[i], minBound[i]) || Math::isEqual(point[i], maxBound[i]);
        }
        return bIsOnBorder ? 2u : 1u;
    }
    uint8 encloses(const Box<T, d> &other)
    {
        PointElementType thisVolume = 1, otherVolume = 1;
        for (uint32 i = 0; i < d; i++)
        {
            if (other.minBound[i] < minBound[i] || other.maxBound[i] > maxBound[i])
            {
                return 0;
            }
            thisVolume *= (maxBound[i] - minBound[i]);
            otherVolume *= (other.maxBound[i] - other.minBound[i]);
        }
        return Math::isEqual(thisVolume, otherVolume) ? 2u : 1u;
    }

    T size() const { return maxBound - minBound; }

    T center() const { return (maxBound + minBound) * 0.5f; }

    void boundCorners(ArrayView<T> &corners) const
    {
        uint32 totalCorners = Math::pow(2, d);
        fatalAssertf(corners.size() >= totalCorners, "Corners must be greater or equal to %d", totalCorners);

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
    bool raycast(
        const T &startPoint, const T &dir, float length, float invLength, float &outEnterLength, T &outEnterPoint, float &outExitLength,
        T &outExitPoint
    ) const
    {
        bool parallel[d];
        T invDir;

        for (uint32 i = 0; i < d; i++)
        {
            parallel[i] = (dir[i] == 0);
            invDir[i] = parallel[i] ? 0 : 1 / dir[i];
        }

        return raycastFast(
            startPoint, dir, invDir, length, invLength, &parallel[0], outEnterLength, outEnterPoint, outExitLength, outExitPoint
        );
    }

    /**
     * Box<T, d>::raycastFast
     *
     * Access: public
     *
     * @param const T & startPoint
     * @param const T & dir
     * @param const T & invDir
     * @param float length
     * @param float invLength
     * @param const bool * const parallel
     * @param float & outEnterLength - Fraction of total length at which ray enters the volume. Valid if ray enters the volume
     * @param T & outEnterPoint
     * @param float & outExitLength - Fraction of total length at which ray exits the volume. Valid if ray exits the volume
     * @param T & outExitPoint
     *
     * @return bool
     */
    bool raycastFast(
        const T &startPoint, const T &dir, const T &invDir, float length, float invLength, const bool *const parallel, float &outEnterLength,
        T &outEnterPoint, float &outExitLength, T &outExitPoint
    ) const
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
                else
                {
                    t2 = FLT_MAX;
                    t1 = 0;
                }
            }
            else
            {
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

template <class T>
class Box<T, 1>
{
public:
    using PointType = T;
    using PointElementType = T;
    CONST_EXPR static const uint32 Dim = 1;

    using value_type = PointType;

public:
    T minBound;
    T maxBound;

    Box() = default;

    Box(const T &min, const T &max)
    {
        minBound = min;
        maxBound = max;
    }

    Box(const Box<T, 1> &other)
    {
        minBound = other.minBound;
        maxBound = other.maxBound;
    }

    Box(Box<T, 1> &&other)
    {
        minBound = std::move(other.minBound);
        maxBound = std::move(other.maxBound);
    }

    Box<T, 1> &operator=(const Box<T, 1> &other)
    {
        minBound = other.minBound;
        maxBound = other.maxBound;
        return *this;
    }

    Box<T, 1> operator+(const Box<T, 1> &other)
    {
        Box<T, 1> newBox;
        newBox.minBound = minBound > other.minBound ? other.minBound : minBound;
        newBox.maxBound = maxBound < other.maxBound ? other.maxBound : maxBound;
        return newBox;
    }

    Box<T, 1> operator+(const T &offset)
    {
        Box<T, 1> newBox;
        newBox.minBound = minBound + offset;
        newBox.maxBound = maxBound + offset;
        return newBox;
    }

    void operator+=(const Box<T, 1> &other) { grow(other); }

    void operator+=(const T &dx) { offset(dx); }

    Box<T, 1> &operator=(Box<T, 1> &&other)
    {
        minBound = std::move(other.minBound);
        maxBound = std::move(other.maxBound);
        return *this;
    }

    void reset(const T &min, const T &max)
    {
        minBound = min;
        maxBound = max;
    }

    void offset(const T &offset)
    {
        minBound += offset;
        maxBound += offset;
    }

    void grow(const Box<T, 1> &other)
    {
        minBound = minBound > other.minBound ? other.minBound : minBound;
        maxBound = maxBound < other.maxBound ? other.maxBound : maxBound;
    }
    void grow(const T &point)
    {
        minBound = minBound > point ? point : minBound;
        maxBound = maxBound < point ? point : maxBound;
    }

    bool intersect(const Box<T, 1> &other) const { return !(other.maxBound < minBound || other.minBound > maxBound); }

    Box<T, 1> getIntersectionBox(const Box<T, 1> &other, bool checkAA = true) const
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

    bool isValidAABB() const { return minBound <= maxBound; }

    void fixAABB()
    {
        if (minBound > maxBound)
        {
            // Swap in case of swapped axis
            std::swap(minBound, maxBound);
        }
    }

    bool contains(const T &point) const { return point >= minBound && point <= maxBound; }
    bool contains(const Box<T, 1> &other) const { return other.minBound >= minBound && other.maxBound <= maxBound; }
    uint8 encloses(const T &point) const
    {
        if (point < minBound || point > maxBound)
            return 0;

        return (Math::isEqual(point, minBound) || Math::isEqual(point, maxBound)) ? 2u : 1u;
    }
    uint8 encloses(const Box<T, 1> &other) const
    {
        if (other.minBound < minBound || other.maxBound > maxBound)
            return 0;
        return (Math::isEqual(other.minBound, minBound) && Math::isEqual(other.maxBound, maxBound)) ? 2u : 1u;
    }

    T size() const { return maxBound - minBound; }

    T center() const { return (maxBound + minBound) * 0.5f; }
};

template <typename T>
using ValueRange = Box<T, 1>;

using SizeBox2D = Box<Size2D, 2>;
using SizeBox3D = Box<Size3D, 3>;

using ShortSizeBox2D = Box<ShortSize2D, 2>;

using QuantizedBox2D = Box<Int2D, 2>;
using QuantizedBox3D = Box<Int3D, 3>;

using QuantShortBox2D = Box<Short2D, 2>;

using Rect = Box<Vector2D, 2>;
using AABB = Box<Vector3D, 3>;

template <typename BoxType>
struct IsBox2DType : std::false_type
{};

template <typename Type>
struct IsBox2DType<Box<Type, 2>> : std::true_type
{};

template <typename BoxType>
struct IsBox3DType : std::false_type
{};

template <typename Type>
struct IsBox3DType<Box<Type, 3>> : std::true_type
{};

template <typename BoxType>
struct IsBoxType : std::false_type
{};

template <typename Type, uint32 ExtDim>
struct IsBoxType<Box<Type, ExtDim>> : std::true_type
{};

template <typename BoxType>
concept Box2DType = IsBox2DType<BoxType>::value;

template <typename BoxType>
concept Box3DType = IsBox3DType<BoxType>::value;