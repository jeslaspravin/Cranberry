/*!
 * \file MathGeom.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Box.h"
#include "Math/CoreMathTypedefs.h"
#include "Math/CoreMathTypes.h"
#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"

template <Box2Dim RectType>
struct PackedRectsBin;

class PROGRAMCORE_EXPORT MathGeom
{
private:
    MathGeom() = default;

public:
    static Vector2 transform2d(const Vector2 &pt, const Vector2 &offset, float rotInDeg);

    /**
     * MathGeom::packRectangles - Packs the given packRects into one or more packed bins, Each bins can
     * take maximum size provided by maxBinRect
     *
     * Access: public static
     *
     * @param std::vector<PackedRectsBin<RectType>> & outPackedBins - Packed bins with all packed in
     * rectangles and it's required size
     * @param const RectType::PointType & maxBinRect
     * @param const std::vector<RectType * > & packRects - must be at origin, so maxBound will be its
     * size
     *
     * @return NODISCARD bool
     */
    template <typename Type, Box2Dim RectType = Box<Type, 2>>
    NODISCARD static bool
    packRectangles(std::vector<PackedRectsBin<RectType>> &outPackedBins, const Type &maxBinRect, const std::vector<RectType *> &packRects);
};

#include "Math/PackRectangles.inl"
