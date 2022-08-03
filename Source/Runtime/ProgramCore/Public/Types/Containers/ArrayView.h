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
public:
    using value_type = ElementType;
    using pointer = ElementType *;
    using const_pointer = ElementType const *;
    using reference = ElementType &;
    using const_reference = ElementType const &;

    template <bool bIsConst>
    class Iterator
    {
    public:
        using value_type = std::conditional_t<bIsConst, const ElementType, ElementType>;
        using reference = value_type &;
        using pointer = value_type *;
        using difference_type = int64;
        using iterator_concept = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;

    private:
        const ArrayView *arrayView;
        SizeT idx;

    public:
        Iterator() = delete;
        Iterator(const Iterator &) = default;
        Iterator(Iterator &&) = default;
        Iterator &operator=(const Iterator &) = default;
        Iterator &operator=(Iterator &&) = default;
        Iterator(const ArrayView &view, SizeT itrIdx)
            : arrayView(&view)
            , idx(itrIdx)
        {
            fatalAssertf(arrayView && idx >= 0 && arrayView->length >= idx, "Invalid iterator data");
        }

        pointer operator->() const noexcept
        {
            debugAssert(arrayView && idx >= 0 && arrayView->length > idx);
            return arrayView->dataPtr + arrayView->offset + idx;
        }

        NODISCARD reference operator*() const noexcept
        {
            debugAssert(arrayView && idx >= 0 && arrayView->length > idx);
            return *(arrayView->dataPtr + arrayView->offset + idx);
        }

        bool operator!=(const Iterator &other) const noexcept { return arrayView != other.arrayView || idx != other.idx; }

        Iterator &operator++() noexcept
        {
            ++idx;
            return *this;
        }

        NODISCARD Iterator operator++(int) noexcept
        {
            Iterator retVal(*arrayView, idx);
            ++idx;
            return retVal;
        }

        Iterator &operator--() noexcept
        {
            --idx;
            return *this;
        }

        NODISCARD Iterator operator--(int) noexcept
        {
            Iterator retVal(*arrayView, idx);
            --idx;
            return retVal;
        }

        Iterator &operator+=(const difference_type off) noexcept
        {
            idx += off;
            return *this;
        }

        NODISCARD Iterator operator+(const difference_type off) const noexcept { return Iterator(*arrayView, idx + off); }

        Iterator &operator-=(const difference_type off) noexcept
        {
            idx -= off;
            return *this;
        }

        NODISCARD Iterator operator-(const difference_type off) const noexcept { return Iterator(*arrayView, idx - off); }

        NODISCARD difference_type operator-(const Iterator &other) const noexcept { return idx - other.idx; }

        NODISCARD reference operator[](const difference_type off) const noexcept
        {
            debugAssert(arrayView && (idx + off) >= 0 && arrayView->length > (idx + off));
            return *(arrayView->dataPtr + arrayView->offset + (idx + off));
        }
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<Iterator<false>>;
    using const_reverse_iterator = std::reverse_iterator<Iterator<true>>;

private:
    pointer dataPtr;
    SizeT offset;
    SizeT length;

public:
    ArrayView()
        : dataPtr(nullptr)
        , offset(0)
        , length(0)
    {}

    template <typename T>
    ArrayView(std::vector<T> &parent, SizeT inOffset = 0)
        : dataPtr(parent.data())
        , offset(Math::min(inOffset, SizeT(parent.size() - 1)))
        , length(SizeT(parent.size() - offset))
    {}

    template <typename T>
    ArrayView(std::vector<T> &parent, SizeT inLength, SizeT inOffset = 0)
        : dataPtr(parent.data())
        , offset(Math::min(inOffset, SizeT(parent.size() - 1)))
        , length(Math::min(inLength, SizeT(parent.size() - offset)))
    {}

    CONST_EXPR ArrayView(pointer parentData, SizeT parentSize, SizeT inOffset = 0)
        : dataPtr(parentData)
        , offset(Math::min(inOffset, parentSize - 1))
        , length(parentSize - offset)
    {}

    template <typename T, SizeT N>
    CONST_EXPR ArrayView(T (&parentData)[N])
        : dataPtr(parentData)
        , offset(0)
        , length(N)
    {}

    CONST_EXPR SizeT size() const { return length; }
    NODISCARD bool empty() const { return length == 0; }
    CONST_EXPR pointer data() { return dataPtr + offset; }
    CONST_EXPR const_pointer data() const { return dataPtr + offset; }
    CONST_EXPR pointer ptr() { return dataPtr; }
    CONST_EXPR const_pointer ptr() const { return dataPtr; }

    CONST_EXPR void reset()
    {
        offset = 0;
        length = 0;
        dataPtr = nullptr;
    }

    reference operator[](SizeT idx)
    {
        fatalAssertf(idx < length, "Invalid index %d", idx);
        return dataPtr[offset + idx];
    }

    const_reference operator[](SizeT idx) const
    {
        fatalAssertf(idx < length, "Invalid index %d", idx);
        return dataPtr[offset + idx];
    }

    NODISCARD iterator begin() noexcept { return iterator(*this, 0); }
    NODISCARD const_iterator begin() const noexcept { return const_iterator(*this, 0); }
    NODISCARD iterator end() noexcept { return iterator(*this, length); }
    NODISCARD const_iterator end() const noexcept { return const_iterator(*this, length); }
    NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    NODISCARD const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    NODISCARD const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    NODISCARD const_iterator cbegin() const noexcept { return begin(); }
    NODISCARD const_iterator cend() const noexcept { return end(); }
    NODISCARD const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    NODISCARD const_reverse_iterator crend() const noexcept { return rend(); }
};