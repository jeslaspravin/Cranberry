/*!
 * \file ArrayView.h
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
#include "Types/Platform/PlatformAssertionErrors.h"

#include <vector>

template <typename ElementType>
class ArrayView
{
private:
    ElementType* dataPtr;
    SizeT offset;
    SizeT length;

public:

    ArrayView()
        : dataPtr(nullptr)
        , offset(0)
        , length(0)
    {}

    ArrayView(std::vector<ElementType>& parent, SizeT inOffset = 0)
    {
        dataPtr = parent.data();
        offset = Math::min(inOffset, SizeT(parent.size() - 1));
        length = SizeT(parent.size() - offset);
    }

    ArrayView(std::vector<ElementType>& parent, SizeT inLength, SizeT inOffset = 0)
    {
        dataPtr = parent.data();
        offset = Math::min(inOffset, SizeT(parent.size() - 1));
        length = Math::min(inLength, SizeT(parent.size() - offset));
    }
    
    ArrayView(ElementType* parentData, SizeT parentSize, SizeT inOffset = 0)
    {
        dataPtr = parentData;
        offset = Math::min(inOffset, parentSize - 1);
        length = parentSize - offset;
    }

    SizeT size() const
    {
        return length;
    }

    NODISCARD bool empty() const
    {
        return length == 0;
    }

    ElementType* data()
    {
        return dataPtr + offset;
    }

    const ElementType* data() const
    {
        return dataPtr + offset;
    }

    ElementType& operator[](SizeT idx)
    {
        fatalAssert(idx < length, "Invalid index %d", idx);
        return dataPtr[offset + idx];
    }

    const ElementType& operator[](SizeT idx) const
    {
        fatalAssert(idx < length, "Invalid index %d", idx);
        return dataPtr[offset + idx];
    }
};