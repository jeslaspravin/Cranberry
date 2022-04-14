/*!
 * \file SparseVector.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/PlatformAssertionErrors.h"

#include <vector>

// SparsityPolicyType - Provides the functions and data that tracks the sparse elements in the vector
// Example implementations are in BitArraySparsityPolicy and FwdListSparsityPolicy.h
template <typename ElementType, typename SparsityPolicyType>
class SparseVector
{
public:
    using ValueType = ElementType;
    using ContainerType = std::vector<ValueType>;
    using SizeType = typename ContainerType::size_type;
    using DifferenceType = typename ContainerType::difference_type;
    using SparsityPolicy = SparsityPolicyType;

    using value_type = ValueType;
    using size_type = SizeType;
    using difference_type = DifferenceType;
    using const_reference = const value_type &;
    using reference = value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    // using IteratorType = ;

    // using iterator = IteratorType<false>;
    // using const_iterator = IteratorType<true>;
    // using reverse_iterator = std::reverse_iterator<IteratorType<false>>;
    // using const_reverse_iterator = std::reverse_iterator<IteratorType<true>>;

    ContainerType elements;
    SparsityPolicy freeSlots;

private:
    void markAllFree()
    {
        freeSlots.resize(elements.size(), true);
        for (SizeType i = 0; i < elements.size(); ++i)
        {
            freeSlots.set(i);
        }
    }
    void markAllOccupied()
    {
        freeSlots.resize(elements.size());
        for (SizeType i = 0; i < elements.size(); ++i)
        {
            freeSlots.reset(i);
        }
    }

public:
    SparseVector() = default;
    SparseVector(const SparseVector &) = default;
    SparseVector(SparseVector &&) = default;

    SparseVector(SizeType count, const ValueType &value) noexcept
        : elements(count, value)
    {
        markAllFree();
    }
    SparseVector(SizeType count) noexcept
        : elements(count)
    {
        markAllOccupied();
    }
    SparseVector(const std::vector<ValueType> &values) noexcept
        : elements(values)
    {
        markAllOccupied();
    }
    SparseVector(std::initializer_list<ValueType> values) noexcept
        : elements(values)
    {
        markAllOccupied();
    }
    template <typename InitItr>
    SparseVector(InitItr beginItr, InitItr endItr) noexcept
        : elements(beginItr, endItr)
    {
        markAllOccupied();
    }
    SparseVector &operator=(const std::vector<ValueType> &values) noexcept
    {
        elements = values;
        markAllOccupied();
        return (*this);
    }
    SparseVector &operator=(std::initializer_list<ValueType> values) noexcept
    {
        elements = values;
        markAllOccupied();
        return (*this);
    }

    reference operator[](SizeType index) noexcept
    {
        fatalAssert(isValid(index), "Index %llu is invalid", index);
        return elements[index];
    }

    const_reference operator[](SizeType index) const noexcept
    {
        fatalAssert(isValid(index), "Index %llu is invalid", index);
        return elements[index];
    }

    // Additional functions
    template <class... ConstructArgs>
    SizeType get(ConstructArgs &&...args)
    {
        SizeType index;
        if (freeSlots.empty())
        {
            index = elements.size();
            elements.emplace_back(std::forward<ConstructArgs>(args)...);
            freeSlots.resize(elements.size());
            freeSlots.set(index);
        }
        else
        {
            index = freeSlots.pop_free();
            new (&elements[index]) ElementType(std::forward<ConstructArgs>(args)...);
        }
        return index;
    }

    bool isValid(SizeType index) const { return elements.size() > index && !freeSlots.isFree(index); }
    SizeType size() const { return elements.size() - freeSlots.size(); }
    // Sum of valid and free elements
    SizeType totalCount() const { return elements.size(); }
    NODISCARD FORCE_INLINE bool empty() const { return size() == 0; }

    void reset(SizeType index)
    {
        fatalAssert(isValid(index), "Index %llu is invalid", index);
        if CONST_EXPR (std::is_destructible_v<ValueType>)
        {
            elements[index].~ElementType();
        }

        freeSlots.push_free(index);
    }

    void clear(SizeType PreserveSize = 0)
    {
        elements.clear();
        elements.reserve(PreserveSize);
        freeSlots.clear();
        freeSlots.reserve(PreserveSize);
    }
};