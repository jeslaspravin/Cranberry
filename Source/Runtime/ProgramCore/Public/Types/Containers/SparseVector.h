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
#include "Math/Math.h"

#include <vector>

template <typename ElementType, typename SparsityPolicyType>
class SparseVectorIterator;

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

    using iterator = SparseVectorIterator<ValueType, SparsityPolicy>;
    using const_iterator = SparseVectorIterator<const ValueType, SparsityPolicy>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    ContainerType elements;
    SparsityPolicy freeSlots;

private:
    void markAllFree()
    {
        freeSlots.resize(elements.size(), true);
        for (SizeType i = 0; i < elements.size(); ++i)
        {
            freeSlots.reset(i);
        }
    }
    void markAllOccupied()
    {
        freeSlots.resize(elements.size());
        for (SizeType i = 0; i < elements.size(); ++i)
        {
            freeSlots.set(i);
        }
    }

public:
    SparseVector() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(SparseVector)

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
    SparseVector(std::vector<ValueType> &&values) noexcept
        : elements(std::move(values))
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
    SparseVector &operator= (const std::vector<ValueType> &values) noexcept
    {
        elements = values;
        markAllOccupied();
        return (*this);
    }
    SparseVector &operator= (std::vector<ValueType> &&values) noexcept
    {
        elements = std::move(values);
        markAllOccupied();
        return (*this);
    }
    SparseVector &operator= (std::initializer_list<ValueType> values) noexcept
    {
        elements = values;
        markAllOccupied();
        return (*this);
    }

    reference operator[] (SizeType index) noexcept
    {
        fatalAssertf(isValid(index), "Index {} is invalid", index);
        return elements[index];
    }

    const_reference operator[] (SizeType index) const noexcept
    {
        fatalAssertf(isValid(index), "Index {} is invalid", index);
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
        fatalAssertf(isValid(index), "Index {} is invalid", index);
        if CONST_EXPR (std::is_destructible_v<ValueType>)
        {
            elements[index].~ElementType();
        }

        freeSlots.push_free(index);
    }
    template <typename ItrValueType>
    auto reset(SparseVectorIterator<ItrValueType, SparsityPolicy> itr);

    void clear(SizeType PreserveSize = 0)
    {
        elements.clear();
        elements.reserve(PreserveSize);
        freeSlots.clear();
        freeSlots.reserve(PreserveSize);
    }

    // Iterators
    NODISCARD CONST_EXPR iterator begin() noexcept;
    NODISCARD CONST_EXPR const_iterator begin() const noexcept;
    NODISCARD CONST_EXPR iterator end() noexcept;
    NODISCARD CONST_EXPR const_iterator end() const noexcept;

    NODISCARD CONST_EXPR reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    NODISCARD CONST_EXPR const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    NODISCARD CONST_EXPR reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    NODISCARD CONST_EXPR const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    NODISCARD CONST_EXPR const_iterator cbegin() const noexcept { return begin(); }
    NODISCARD CONST_EXPR const_iterator cend() const noexcept { return end(); }
    NODISCARD CONST_EXPR const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    NODISCARD CONST_EXPR const_reverse_iterator crend() const noexcept { return rend(); }
};

template <typename ElementType, typename SparsityPolicyType>
class SparseVectorIterator
{
public:
    constexpr static const bool bIsConst = std::is_const_v<ElementType>;

    using SparseVectorType = SparseVector<std::remove_const_t<ElementType>, SparsityPolicyType>;
    using ContainerConstCorrectType = std::conditional_t<bIsConst, const SparseVectorType, SparseVectorType>;
    using SizeType = typename SparseVectorType::size_type;

    using value_type = std::conditional_t<bIsConst, typename SparseVectorType::value_type const, typename SparseVectorType::value_type>;
    using reference = value_type &;
    using pointer = value_type *;
    using size_type = SizeType;
    using difference_type = typename SparseVectorType::difference_type;
    using iterator_concept = typename std::random_access_iterator_tag;
    using iterator_category = typename std::random_access_iterator_tag;

private:
    SizeType idx;
    ContainerConstCorrectType *iteratingVector;

private:
    FORCE_INLINE void validateItr() const { fatalAssertf(iteratingVector && iteratingVector->isValid(idx), "Iterator is invalid!"); }

public:
    CONST_EXPR SparseVectorIterator() = delete;
    CONST_EXPR SparseVectorIterator(const SparseVectorIterator &) = default;
    CONST_EXPR SparseVectorIterator(SparseVectorIterator &&) = default;
    CONST_EXPR SparseVectorIterator &operator= (const SparseVectorIterator &) = default;
    CONST_EXPR SparseVectorIterator &operator= (SparseVectorIterator &&) = default;
    CONST_EXPR SparseVectorIterator(SizeType startIdx, ContainerConstCorrectType &sparseVector)
        : idx(startIdx)
        , iteratingVector(&sparseVector)
    {
        // If not acceptable invalid index then go to next valid index
        while (!iteratingVector->isValid(idx) && idx < iteratingVector->totalCount())
        {
            ++idx;
        }
        idx = Math::min(idx, iteratingVector->totalCount());
    }

    CONST_EXPR pointer operator->() const noexcept
    {
        validateItr();
        return &(*iteratingVector)[idx];
    }
    NODISCARD CONST_EXPR reference operator* () const noexcept
    {
        validateItr();
        return (*iteratingVector)[idx];
    }

    CONST_EXPR bool operator!= (const SparseVectorIterator &other) const noexcept
    {
        return iteratingVector != other.iteratingVector || idx != other.idx;
    }

    CONST_EXPR SparseVectorIterator &operator++ () noexcept
    {
        validateItr();
        while (!iteratingVector->isValid(++idx) && idx < iteratingVector->totalCount())
        {}
        idx = Math::min(idx, iteratingVector->totalCount());
        return *this;
    }

    NODISCARD CONST_EXPR SparseVectorIterator operator++ (int) noexcept
    {
        SparseVectorIterator retVal(*this);
        this->operator++ ();
        return retVal;
    }

    CONST_EXPR SparseVectorIterator &operator-- () noexcept
    {
        validateItr();
        while (!iteratingVector->isValid(--idx) && idx > 0)
        {}
        idx = Math::max(idx, 0);
        return *this;
    }

    NODISCARD CONST_EXPR SparseVectorIterator operator-- (int) noexcept
    {
        SparseVectorIterator retVal(*this);
        this->operator-- ();
        return retVal;
    }

    CONST_EXPR SparseVectorIterator &operator+= (const difference_type off) noexcept
    {
        if (off >= 0)
        {
            while (off != 0)
            {
                this->operator++ ();
                off--;
            }
        }
        else
        {
            while (off != 0)
            {
                this->operator-- ();
                off++;
            }
        }
        return *this;
    }

    NODISCARD CONST_EXPR SparseVectorIterator operator+ (const difference_type off) const noexcept
    {
        SparseVectorIterator retVal(*this);
        retVal += off;
        return retVal;
    }

    CONST_EXPR SparseVectorIterator &operator-= (const difference_type off) noexcept
    {
        (*this) += (-off);
        return *this;
    }

    NODISCARD CONST_EXPR SparseVectorIterator operator- (const difference_type off) const noexcept
    {
        SparseVectorIterator retVal(*this);
        retVal -= off;
        return retVal;
    }
    NODISCARD CONST_EXPR difference_type operator- (const SparseVectorIterator &other) const noexcept
    {
        if (other->idx > idx)
        {
            return -(other - (*this));
        }

        debugAssert(iteratingVector && other.iteratingVector && iteratingVector == other.iteratingVector);
        difference_type diff = 0;
        size_type tempIdx = other->idx;
        while (tempIdx < idx)
        {
            if (iteratingVector->isValid(tempIdx))
            {
                diff++;
            }
            tempIdx++;
        }
        return diff;
    }

    NODISCARD CONST_EXPR value_type operator[] (const difference_type off) const noexcept
    {
        SparseVectorIterator retVal(*this);
        retVal += off;
        return *retVal;
    }

    void reset() noexcept
    {
        SizeType oldIdx = idx;
        this->operator++ ();
        iteratingVector->reset(oldIdx);
    }
};

template <typename ElementType, typename SparsityPolicyType>
NODISCARD CONST_EXPR typename SparseVector<ElementType, SparsityPolicyType>::iterator
SparseVector<ElementType, SparsityPolicyType>::begin() noexcept
{
    return iterator(0, *this);
}

template <typename ElementType, typename SparsityPolicyType>
NODISCARD CONST_EXPR typename SparseVector<ElementType, SparsityPolicyType>::const_iterator
SparseVector<ElementType, SparsityPolicyType>::begin() const noexcept
{
    return const_iterator(0, *this);
}

template <typename ElementType, typename SparsityPolicyType>
NODISCARD CONST_EXPR typename SparseVector<ElementType, SparsityPolicyType>::const_iterator
SparseVector<ElementType, SparsityPolicyType>::end() const noexcept
{
    return const_iterator(totalCount(), *this);
}

template <typename ElementType, typename SparsityPolicyType>
NODISCARD CONST_EXPR typename SparseVector<ElementType, SparsityPolicyType>::iterator
SparseVector<ElementType, SparsityPolicyType>::end() noexcept
{
    return iterator(totalCount(), *this);
}

template <typename ElementType, typename SparsityPolicyType>
template <typename ItrValueType>
auto SparseVector<ElementType, SparsityPolicyType>::reset(
    SparseVectorIterator<ItrValueType, SparseVector<ElementType, SparsityPolicyType>::SparsityPolicy> itr
)
{
    itr.reset();
    return itr;
}