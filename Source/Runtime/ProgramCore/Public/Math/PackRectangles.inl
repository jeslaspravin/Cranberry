/*!
 * \file PackRectangles.inl
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Reflections/Functions.h"

template <Box2DType RectType>
struct PackedRectsBin
{
    std::vector<RectType*> rects;
    RectType::PointType binSize;
};


#define RECT_TYPE_SIZE(RectVal) ((RectVal).maxBound)
#define RECT_TYPE_AREA(RectVal) (RECT_TYPE_SIZE(RectVal).x * RECT_TYPE_SIZE(RectVal).x)
#define RECT_SIZE_AREA(RectSize) ((RectSize).x * (RectSize).y)

// Based on https://blackpawn.com/texts/lightmaps/default.html
// and inspiration from https://github.com/turanszkij/WickedEngine/blob/master/WickedEngine/wiRectPacker.cpp#L48 
template <Box2DType RectType>
class RectPacker
{
public:
    using RectPointType = typename RectType::PointType;
    using RectCompType = typename RectPointType::value_type;

private:
    RectPacker() = default;

    struct Node
    {
        // If child[0] is not null then child[1] must be not null
        // If both are null it means that rectangle is free to be filled
        // If child[0] is null and child[1] is not null then this node is taken and cannot insert
        Node* childs[2] = { nullptr, nullptr };
        RectType rect;

        const Node* insert(const RectPointType& inRectSize);
        void reset(const RectPointType& rectSize)
        {
            rect.minBound = RectPointType(0);
            rect.maxBound = rectSize;
            if (childs[0] != nullptr)
            {
                delete childs[0];
                delete childs[1];
            }
            childs[0] = nullptr;
            childs[1] = nullptr;
        }
        ~Node()
        {
            if (childs[0] != nullptr)
            {
                delete childs[0];
                delete childs[1];
            }
        }
    };

    static bool areaCompare(const RectType* lhs, const RectType* rhs)
    {
        return RECT_TYPE_AREA(*lhs) > RECT_TYPE_AREA(*rhs);
    }
    static bool widthCompare(const RectType* lhs, const RectType* rhs)
    {
        return RECT_TYPE_SIZE(*lhs).x > RECT_TYPE_SIZE(*rhs).x;
    }
    static bool heightCompare(const RectType* lhs, const RectType* rhs)
    {
        return RECT_TYPE_SIZE(*lhs).y > RECT_TYPE_SIZE(*rhs).y;
    }
    static bool maxSideCompare(const RectType* lhs, const RectType* rhs)
    {
        return (Math::max(RECT_TYPE_SIZE(*lhs).x, RECT_TYPE_SIZE(*lhs).y) > Math::max(RECT_TYPE_SIZE(*rhs).x, RECT_TYPE_SIZE(*rhs).y));
    }

    using CompareRectFuncType = Function<bool, const RectType*, const RectType*>;
    CONST_EXPR static const CompareRectFuncType COMPARE_FUNCS[] = {
        &areaCompare
        , &widthCompare
        , &heightCompare
        , &maxSideCompare
    };
    // when there is successful packing, This will be the minimum step at which further smaller packing will be stopped
    CONST_EXPR static const RectCompType DISCARD_AT_STEP = 128 * 128;
    // Tries to pack all the given rectangles within given bin rectangle and returns true if successful
    // or false if no packing can be done in that limit
    // outBestBinRect contains best successfully packed rectangle and outMaxPackedArea gives max packable area for given bin rectangle 
    static bool getBestPackProps(RectPointType& outBestBinRect, RectCompType& outMaxPackedArea
        , const ArrayView<RectType*>& inRects, const RectPointType& maxBinRect);
public:
    // inRects must be at origin, so maxBound will be its size
    static void pack(PackedRectsBin<RectType>& outBin, std::vector<RectType*>& failedRects, std::vector<RectType*>& inRects, const RectPointType& maxBinRect);
};

template <typename Type, Box2DType RectType /*= Box<Type, 2>*/>
bool MathGeom::packRectangles(std::vector<PackedRectsBin<RectType>>& outPackedBins, const Type& maxBinRect, const std::vector<RectType*>& packRects)
{
    RectType binRect(RectType::PointType(0), maxBinRect);
    for (const RectType* rect : packRects)
    {
        if (!binRect.contains(*rect))
        {
            return false;
        }
    }

    std::vector<RectType*> rects0, rects1, * rectsToPack, * failedRects;
    rects1.reserve(packRects.size());
    rects0.resize(packRects.size());
    memcpy(rects0.data(), packRects.data(), sizeof(RectType*) * packRects.size());
    rectsToPack = &rects0;
    failedRects = &rects1;
    while (true)
    {
        PackedRectsBin<RectType>& packBin = outPackedBins.emplace_back();
        RectPacker<RectType>::pack(packBin, *failedRects, *rectsToPack, maxBinRect);

        std::swap(failedRects, rectsToPack);
        if (rectsToPack->empty())
        {
            break;
        }
        failedRects->clear();
        failedRects->reserve(rectsToPack->size());
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
/// RectPacker<RectType> implementations
//////////////////////////////////////////////////////////////////////////

// Should we insert all rect top down vs doing bottom up as per https://blackpawn.com/texts/lightmaps/default.html 
// Top down inserts rectangles from left top(0, 0) of rectangle
// Bottom up inserts rectangles from right bottom(1, 1) of rectangle
// Both creates similar insert flow and fragmentations
#define RECT_PACKER_INSERT_TOPDOWN 1

template <Box2DType RectType>
const typename RectPacker<RectType>::Node* RectPacker<RectType>::Node::insert(const RectPointType& inRectSize)
{
    // Children are present try inserting to them instead
    if (childs[0] != nullptr)
    {
        if (const Node* retVal = childs[0]->insert(inRectSize))
        {
            return retVal;
        }
        return childs[1]->insert(inRectSize);
    }

    // Already filled rectangle
    if (childs[1] != nullptr)
    {
        return nullptr;
    }

    RectType inRect{ rect.minBound, rect.minBound + inRectSize };
    uint8 enclosable = rect.encloses(inRect);

    // Cannot contain inRect
    if (enclosable == 0)
    {
        return nullptr;
    }

    // Perfect fit
    if (enclosable == 2)
    {
        childs[0] = nullptr;
        childs[1] = (Node*)(0xFF);// Just some random none 0 value that we are not going to dereference
        rect = inRect;
        return this;
    }

    childs[0] = new Node;
    childs[1] = new Node;
    // Why diff and not use inRectSize? If inRectSize is a square then the split always goes in one way and causes infinite recursion
    RectPointType diff = rect.size() - inRectSize;
#if RECT_PACKER_INSERT_TOPDOWN
    if (diff.x > diff.y)
    {
        // Remaining width is larger than remaining height so splitting along width to get small fitting area in child 0
        childs[0]->rect = RectType{
            rect.minBound
            , RectPointType{ inRect.maxBound.x, rect.maxBound.y }
        };
        childs[1]->rect = RectType{
            RectPointType{ inRect.maxBound.x, rect.minBound.y }
            , rect.maxBound
        };
    }
    else
    {
        // Remaining height is larger than remaining width so splitting along height to get small fitting area in child 0
        childs[0]->rect = RectType{
            rect.minBound
            , RectPointType{ rect.maxBound.x, inRect.maxBound.y }
        };
        childs[1]->rect = RectType{
            RectPointType{ rect.minBound.x, inRect.maxBound.y }
            , rect.maxBound
        };
    }
#else // RECT_PACKER_INSERT_TOPDOWN
    if (diff.x > diff.y)
    {
        // Remaining width is larger than remaining height so splitting along width and making right rectangle as small fitting area in child 0
        childs[0]->rect = RectType{
            RectPointType{ rect.maxBound.x - inRectSize.x, rect.minBound.y }
            , rect.maxBound
        };
        childs[1]->rect = RectType{
            rect.minBound
            , RectPointType{ rect.maxBound.x - inRectSize.x, rect.maxBound.y }
        };
    }
    else
    {
        // Remaining height is larger than remaining width so splitting along height and making bottom rect as small fitting area in child 0
        childs[0]->rect = RectType{
            RectPointType{ rect.minBound.x, rect.maxBound.y - inRectSize.y }
            , rect.maxBound
        };
        childs[1]->rect = RectType{
            rect.minBound
            , RectPointType{ rect.maxBound.x, rect.maxBound.y - inRectSize.y }
        };
    }
#endif // RECT_PACKER_INSERT_TOPDOWN
    return childs[0]->insert(inRectSize);
}

#undef RECT_PACKER_INSERT_TOPDOWN

template <Box2DType RectType>
void RectPacker<RectType>::pack(PackedRectsBin<RectType>& outBin, std::vector<RectType*>& failedRects
    , std::vector<RectType*>& inRects, const RectPointType& maxBinRect)
{
    Node root{ .rect = { RectPointType{0}, maxBinRect } };
#if 0 // Basic insert
    for (RectType* rect : inRects)
    {
        const Node* insertedNode = root.insert(RECT_TYPE_SIZE(*rect));
        if (insertedNode)
        {
            (*rect) = insertedNode->rect;
            outBin.rects.emplace_back(rect);
        }
        else
        {
            failedRects.emplace_back(rect);
        }
    }
#endif 
    RectType** bestSortedRects = nullptr;
    // Find sorting with best packing
    {
        std::array<RectType**, ARRAY_LENGTH(COMPARE_FUNCS)> sortedRects;
        for (int32 i = 0; i < ARRAY_LENGTH(COMPARE_FUNCS); ++i)
        {
            sortedRects[i] = new RectType * [inRects.size()];
            memcpy(sortedRects[i], inRects.data(), sizeof(RectType*) * inRects.size());
            std::sort(sortedRects[i], sortedRects[i] + inRects.size(), COMPARE_FUNCS[i]);
        }

        // best sorting function that provided best packing
        int32 bestFunc = -1;
        RectPointType bestBinSize = maxBinRect;
        // sorting function that provided largest packed area in case of all failed to pack
        int32 maxAreaFunc = -1;
        RectCompType maxArea = 0;

        for (int32 i = 0; i < ARRAY_LENGTH(COMPARE_FUNCS); ++i)
        {
            RectPointType binSize;
            RectCompType packedArea;
            bool bSuccess = getBestPackProps(binSize
                , packedArea
                , ArrayView<RectType*>(sortedRects[i], inRects.size())
                , bestBinSize);
            if (bSuccess && (RECT_SIZE_AREA(bestBinSize) > RECT_SIZE_AREA(binSize)))
            {
                bestFunc = i;
                bestBinSize = binSize;
            }
            else if (!bSuccess && (maxArea < packedArea))
            {
                maxArea = packedArea;
                maxAreaFunc = i;
            }
        }

        int32 bestSortedFunc = (bestFunc >= 0) ? bestFunc : maxAreaFunc;
        bestSortedRects = sortedRects[bestSortedFunc];
        root.reset(bestBinSize);
        // Delete the rest
        for (int32 i = 0; i < ARRAY_LENGTH(COMPARE_FUNCS); ++i)
        {
            if (i != bestSortedFunc)
            {
                delete[] sortedRects[i];
            }
        }
    }

    outBin.binSize = RECT_TYPE_SIZE(root.rect);
    for (uint32 rectIdx = 0; rectIdx < inRects.size(); ++rectIdx)
    {
        RectType* rect = bestSortedRects[rectIdx];

        const Node* insertedNode = root.insert(RECT_TYPE_SIZE(*rect));
        if (insertedNode)
        {
            (*rect) = insertedNode->rect;
            outBin.rects.emplace_back(rect);
        }
        else
        {
            failedRects.emplace_back(rect);
        }
    }
    delete[] bestSortedRects;
}

template <Box2DType RectType>
bool RectPacker<RectType>::getBestPackProps(RectPointType& outBestBinRect
    , RectCompType& outMaxPackedArea, const ArrayView<RectType*>& inRects, const RectPointType& maxBinRect)
{
    outBestBinRect = RectPointType(0);
    outMaxPackedArea = 0;

    Node root{ .rect = { RectPointType{0}, maxBinRect } };
    // step will be abs size to step either up or down based on if packing is successful into certain rectangle
    RectPointType step{ maxBinRect / RectCompType(2) };

    bool bSuccess = true;
    while (true)
    {
        // Failure to pack case
        if (RECT_TYPE_AREA(root.rect) > RECT_SIZE_AREA(maxBinRect))
        {
            bSuccess = false;
            root.reset(maxBinRect);

            for (uint32 i = 0; i < inRects.size(); ++i)
            {
                RectType* rect = inRects[i];
                if (root.insert(RECT_TYPE_SIZE(*rect)))
                {
                    outMaxPackedArea += RECT_TYPE_AREA(*rect);
                }
            }
            break;
        }

        // Not using multiplier -1 because that messes with unsigned rectangle values
        bool packed = true;
        for (uint32 i = 0; i < inRects.size(); ++i)
        {
            RectType* rect = inRects[i];
            // If we failed try increasing the bin rect
            if (!root.insert(RECT_TYPE_SIZE(*rect)))
            {
                packed = false;
                break;
            }
        }

        // If we succeeded this packing and if step dropped below threshold, We force quit to avoid ping pong up to 1
        if (packed && RECT_SIZE_AREA(step) < DISCARD_AT_STEP)
        {
            outBestBinRect = RECT_TYPE_SIZE(root.rect);
            break;
        }

        RectPointType rectSize{ RECT_TYPE_SIZE(root.rect) };
        if (packed)
        {
            rectSize -= step;
        }
        else
        {
            rectSize += step;
        }
        root.reset(rectSize);
        step = Math::max(step / RectCompType(2), RectPointType(1));
    }
    return bSuccess;
}

#undef RECT_TYPE_SIZE
#undef RECT_TYPE_AREA
#undef RECT_SIZE_AREA