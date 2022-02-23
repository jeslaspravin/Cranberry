/*!
 * \file MathGeom.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/PlatformAssertionErrors.h"
#include "Math/Math.h"
#include "Math/Box.h"
#include "Math/CoreMathTypes.h"
#include "Math/CoreMathTypedefs.h"

template <Box2DType RectType>
struct PackedRectsBin;

class PROGRAMCORE_EXPORT MathGeom
{
private:
    MathGeom() = default;

public:

    static Vector2D transform2d(const Vector2D& pt, const Vector2D& offset, float rotInDeg);
    
    /**
    * MathGeom::packRectangles - Packs the given packRects into one or more packed bins, Each bins can take maximum size provided by maxBinRect
    *
    * Access: public static  
    *
    * @param std::vector<PackedRectsBin<RectType>> & outPackedBins - Packed bins with all packed in rectangles and it's required size
    * @param const RectType::PointType & maxBinRect 
    * @param const std::vector<RectType * > & packRects - must be at origin, so maxBound will be its size
    *
    * @return NODISCARD bool
    */
    template <typename Type, Box2DType RectType = Box<Type, 2>>
    NODISCARD static bool packRectangles(std::vector<PackedRectsBin<RectType>>& outPackedBins, const Type& maxBinRect, const std::vector<RectType*>& packRects);
};

#include "Math/PackRectangles.inl"
