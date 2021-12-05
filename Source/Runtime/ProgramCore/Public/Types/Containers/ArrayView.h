#pragma once

#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <vector>

template <typename ElementType>
class ArrayView
{
private:
    ElementType* dataPtr;
    uint32 offset;
    uint32 length;

public:

    ArrayView()
        : dataPtr(nullptr)
        , offset(0)
        , length(0)
    {}

    ArrayView(std::vector<ElementType>& parent, uint32 inOffset = 0)
    {
        dataPtr = parent.data();
        offset = Math::min(inOffset, uint32(parent.size() - 1));
        length = uint32(parent.size() - offset);
    }

    ArrayView(std::vector<ElementType>& parent, uint32 inLength, uint32 inOffset = 0)
    {
        dataPtr = parent.data();
        offset = Math::min(inOffset, uint32(parent.size() - 1));
        length = Math::min(inLength, uint32(parent.size() - offset));
    }
    
    ArrayView(ElementType* parentData, uint32 parentSize, uint32 inOffset = 0)
    {
        dataPtr = parentData;
        offset = Math::min(inOffset, parentSize - 1);
        length = parentSize - offset;
    }

    uint32 size() const
    {
        return length;
    }

    [[nodiscard]] bool empty() const
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

    ElementType& operator[](uint32 idx)
    {
        fatalAssert(idx < length, "Invalid index %d", idx);
        return dataPtr[offset + idx];
    }

    const ElementType& operator[](uint32 idx) const
    {
        fatalAssert(idx < length, "Invalid index %d", idx);
        return dataPtr[offset + idx];
    }
};