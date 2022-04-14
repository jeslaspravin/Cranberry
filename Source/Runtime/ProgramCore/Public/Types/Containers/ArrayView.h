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
    using reference = ElementType &;

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
            fatalAssert(arrayView && idx >= 0 && arrayView->length >= idx, "Invalid iterator data");
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

        bool operator!=(const Iterator &other) const noexcept
        {
            return arrayView != other.arrayView || idx != other.idx;
        }

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

        NODISCARD Iterator operator+(const difference_type off) const noexcept
        {
            return Iterator(*arrayView, idx + off);
        }

        Iterator &operator-=(const difference_type off) noexcept
        {
            idx -= off;
            return *this;
        }

        NODISCARD Iterator operator-(const difference_type off) const noexcept
        {
            return Iterator(*arrayView, idx - off);
        }

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

    ArrayView(std::vector<value_type> &parent, SizeT inOffset = 0)
    {
        dataPtr = parent.data();
        offset = Math::min(inOffset, SizeT(parent.size() - 1));
        length = SizeT(parent.size() - offset);
    }

    ArrayView(std::vector<value_type> &parent, SizeT inLength, SizeT inOffset = 0)
    {
        dataPtr = parent.data();
        offset = Math::min(inOffset, SizeT(parent.size() - 1));
        length = Math::min(inLength, SizeT(parent.size() - offset));
    }

    ArrayView(pointer parentData, SizeT parentSize, SizeT inOffset = 0)
    {
        dataPtr = parentData;
        offset = Math::min(inOffset, parentSize - 1);
        length = parentSize - offset;
    }

    SizeT size() const { return length; }

    NODISCARD bool empty() const { return length == 0; }

    pointer data() { return dataPtr + offset; }

    const pointer data() const { return dataPtr + offset; }

    reference operator[](SizeT idx)
    {
        fatalAssert(idx < length, "Invalid index %d", idx);
        return dataPtr[offset + idx];
    }

    const reference operator[](SizeT idx) const
    {
        fatalAssert(idx < length, "Invalid index %d", idx);
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