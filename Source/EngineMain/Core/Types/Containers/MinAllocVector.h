#pragma once

#include "../../Platform/PlatformAssertionErrors.h"

#include <vector>
#include <forward_list>

template <typename ElementType, typename Enable = std::enable_if_t<std::is_default_constructible_v<ElementType>>>
class MinAllocVector
{
private:
    using ContainerType = std::vector<ElementType>;
    using SizeType = typename ContainerType::size_type;
    ContainerType elements;
    std::forward_list<SizeType> freeSlots;

public:
    SizeType get()
    {
        SizeType index;
        if (freeSlots.empty())
        {
            index = elements.size();
            elements.emplace_back();
        }
        else
        {
            index = freeSlots.front();
            freeSlots.pop_front();
        }
        return index;
    }

    ElementType& operator[](const SizeType& index)
    {
        fatalAssert(elements.size() > index, "Index %d is invalid", index);
        return elements[index];
    }

    const ElementType& operator[](const SizeType& index) const
    {
        fatalAssert(elements.size() > index, "Index %d is invalid", index);
        return elements[index];
    }

    bool isValid(const SizeType& index) const
    {
        return elements.size() > index && std::find(freeSlots.cbegin(), freeSlots.cend(), index) != freeSlots.cend();
    }

    void reset(const SizeType& index)
    {
        fatalAssert(elements.size() > index, "Index %d is invalid", index);
        new (&elements[index])ElementType();

        if (freeSlots.empty() || index >= freeSlots.front())
        {
            freeSlots.emplace_front(index);
            freeSlots.sort();
        }
        else
        {
            freeSlots.emplace_front(index);
        }
    }

    void clear(const SizeType& PreserveSize)
    {
        elements.clear();
        elements.reserve(PreserveSize);
        freeSlots.clear();
    }
};