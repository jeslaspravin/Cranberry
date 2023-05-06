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

template <typename ElemType>
class ArrayView;
template <typename ElemType>
class ArrayRange;

// Have to skip self type here to avoid unnecessarily long compile time
template <typename T, typename ElemType>
concept ArrayViewVectorQualifierInternal
    = IndexableCompound<T> && std::same_as<typename T::value_type, ElemType> && (!std::same_as<T, ArrayView<ElemType>>);
template <typename T, typename ElemType>
concept ArrayRangeVectorQualifierInternal = ArrayViewVectorQualifierInternal<T, ElemType> && (!std::same_as<T, ArrayRange<ElemType>>);

template <typename T, typename ElemType>
concept ArrayViewVectorQualifier = ArrayViewVectorQualifierInternal<std::remove_cvref_t<T>, ElemType>;
template <typename T, typename ElemType>
concept ArrayRangeVectorQualifier = ArrayRangeVectorQualifierInternal<std::remove_cvref_t<T>, ElemType>;

template <typename ElementType, bool bIsConst>
class ArrayIterator
{
public:
    using ValueType = std::conditional_t<bIsConst, const ElementType, ElementType>;
    using RefType = ValueType &;
    using PtrType = ValueType *;

    using value_type = ValueType;
    using reference = RefType;
    using pointer = PtrType;
    using difference_type = int64;
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;

private:
    PtrType dataPtr;
    SizeT idx;

public:
    ArrayIterator() = delete;
    MAKE_TYPE_DEFAULT_COPY_MOVE(ArrayIterator)

    ArrayIterator(PtrType inPtr, SizeT itrIdx) noexcept
        : dataPtr(inPtr)
        , idx(itrIdx)
    {}

    PtrType operator->() const noexcept { return dataPtr + idx; }

    NODISCARD RefType operator* () const noexcept { return dataPtr[idx]; }

    NODISCARD bool operator!= (const ArrayIterator &other) const noexcept { return dataPtr != other.dataPtr || idx != other.idx; }

    ArrayIterator &operator++ () noexcept
    {
        ++idx;
        return *this;
    }
    NODISCARD ArrayIterator operator++ (int) noexcept
    {
        ArrayIterator retVal(dataPtr, idx);
        ++idx;
        return retVal;
    }

    ArrayIterator &operator-- () noexcept
    {
        --idx;
        return *this;
    }
    NODISCARD ArrayIterator operator-- (int) noexcept
    {
        ArrayIterator retVal(dataPtr, idx);
        --idx;
        return retVal;
    }

    ArrayIterator &operator+= (const difference_type off) noexcept
    {
        idx += off;
        return *this;
    }
    NODISCARD ArrayIterator operator+ (const difference_type off) const noexcept { return ArrayIterator(dataPtr, idx + off); }

    ArrayIterator &operator-= (const difference_type off) noexcept
    {
        idx -= off;
        return *this;
    }
    NODISCARD ArrayIterator operator- (const difference_type off) const noexcept { return ArrayIterator(dataPtr, idx - off); }
    NODISCARD difference_type operator- (const ArrayIterator &other) const noexcept { return idx - other.idx; }

    NODISCARD RefType operator[] (const difference_type off) const noexcept { return dataPtr[idx + off]; }
};

/**
 * ArrayView is a const view of array of data.
 */
template <typename ElemType>
class ArrayView
{
    static_assert(!std::is_const_v<ElemType>, "Type name must not be const for ArrayView");

public:
    using ElementType = ElemType;
    using ValueType = const ElementType;
    using RefType = ValueType &;
    using PtrType = ValueType *;

    using value_type = ValueType;
    using pointer = PtrType;
    using const_pointer = PtrType;
    using reference = RefType;
    using const_reference = RefType;

    using iterator = ArrayIterator<ElementType, true>;
    using const_iterator = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    PtrType dataPtr = nullptr;
    SizeT offset = 0;
    SizeT length = 0;

public:
    ArrayView() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(ArrayView)

    // From Array or Vector types
    template <ArrayViewVectorQualifier<ElementType> T>
    ArrayView(const T &parent, SizeT inOffset = 0)
        : dataPtr(parent.data())
        , offset(Math::min(inOffset, SizeT(parent.size() - 1)))
        , length(SizeT(parent.size() - offset))
    {}
    template <ArrayViewVectorQualifier<ElementType> T>
    ArrayView(const T &parent, SizeT inLength, SizeT inOffset = 0)
        : dataPtr(parent.data())
        , offset(Math::min(inOffset, SizeT(parent.size() - 1)))
        , length(Math::min(inLength, SizeT(parent.size() - offset)))
    {}
    template <ArrayViewVectorQualifier<ElementType> T>
    ArrayView &operator= (const T &parent)
    {
        dataPtr = parent.data();
        offset = 0;
        length = parent.size();
        return *this;
    }

    // Directly from pointer
    constexpr ArrayView(PtrType parentData, SizeT parentSize, SizeT inOffset = 0)
        : dataPtr(parentData)
        , offset(Math::min(inOffset, parentSize - 1))
        , length(parentSize - offset)
    {}

    // Directly from constant sized array
    template <SizeT N>
    constexpr ArrayView(const ElementType (&parentData)[N])
        : dataPtr(parentData)
        , offset(0)
        , length(N)
    {}
    template <SizeT N>
    constexpr ArrayView &operator= (const ElementType (&parentData)[N])
    {
        dataPtr = parentData;
        offset = 0;
        length = N;
        return *this;
    }

    // From temporary initializer list
    constexpr ArrayView(std::initializer_list<ElementType> &&tempList)
        : dataPtr(tempList.begin())
        , offset(0)
        , length(tempList.size())
    {}
    constexpr ArrayView &operator= (std::initializer_list<ElementType> &&tempList)
    {
        dataPtr = tempList.begin();
        offset = 0;
        length = tempList.size();
        return *this;
    }

    constexpr SizeT size() const { return length; }
    NODISCARD bool empty() const { return length == 0; }
    constexpr const_pointer data() const { return dataPtr + offset; }
    constexpr const_pointer ptr() const { return dataPtr; }

    constexpr const_reference front() const { return *data(); }
    constexpr const_reference back() const { return *(data() + (length - 1)); }

    constexpr void reset()
    {
        offset = 0;
        length = 0;
        dataPtr = nullptr;
    }

    const_reference operator[] (SizeT idx) const { return dataPtr[offset + idx]; }

    NODISCARD iterator begin() const noexcept { return iterator(dataPtr + offset, 0); }
    NODISCARD iterator end() const noexcept { return iterator(dataPtr + offset, length); }
    NODISCARD reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    NODISCARD reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    NODISCARD const_iterator cbegin() const noexcept { return const_iterator(dataPtr + offset, 0); }
    NODISCARD const_iterator cend() const noexcept { return const_iterator(dataPtr + offset, length); }
    NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }
};

/**
 * ArrayRange is a range in array of data.
 * This holds the non const data. However assumes that it owns the data itself.
 */
template <typename ElemType>
class ArrayRange
{
    static_assert(
        !std::is_const_v<ElemType>, "Type name must not be const for ArrayRange. Use const ArrayRange<ElementType> if need constant behavior"
    );

public:
    using ElementType = ElemType;
    using ValueType = ElementType;
    using RefType = ValueType &;
    using PtrType = ValueType *;

    using value_type = ValueType;
    using pointer = PtrType;
    using const_pointer = ValueType const *;
    using reference = RefType;
    using const_reference = ValueType const &;

    using iterator = ArrayIterator<ElementType, false>;
    using const_iterator = ArrayIterator<ElementType, true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    PtrType dataPtr = nullptr;
    SizeT offset = 0;
    SizeT length = 0;

public:
    ArrayRange() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(ArrayRange)

    // From Array or Vector types
    template <ArrayRangeVectorQualifier<ElementType> T>
    ArrayRange(T &parent, SizeT inOffset = 0)
        : dataPtr(parent.data())
        , offset(Math::min(inOffset, SizeT(parent.size() - 1)))
        , length(SizeT(parent.size() - offset))
    {}
    template <ArrayRangeVectorQualifier<ElementType> T>
    ArrayRange(T &parent, SizeT inLength, SizeT inOffset = 0)
        : dataPtr(parent.data())
        , offset(Math::min(inOffset, SizeT(parent.size() - 1)))
        , length(Math::min(inLength, SizeT(parent.size() - offset)))
    {}
    template <ArrayRangeVectorQualifier<ElementType> T>
    ArrayRange &operator= (T &parent)
    {
        dataPtr = parent.data();
        offset = 0;
        length = parent.size();
        return *this;
    }

    // Directly from pointer
    constexpr ArrayRange(PtrType parentData, SizeT parentSize, SizeT inOffset = 0)
        : dataPtr(parentData)
        , offset(Math::min(inOffset, parentSize - 1))
        , length(parentSize - offset)
    {}

    // Directly from constant sized array
    template <SizeT N>
    constexpr ArrayRange(ElementType (&parentData)[N])
        : dataPtr(parentData)
        , offset(0)
        , length(N)
    {}
    template <SizeT N>
    constexpr ArrayRange &operator= (ElementType (&parentData)[N])
    {
        dataPtr = parentData;
        offset = 0;
        length = N;
        return *this;
    }

    constexpr SizeT size() const { return length; }
    NODISCARD bool empty() const { return length == 0; }

    constexpr pointer data() { return dataPtr + offset; }
    constexpr pointer ptr() { return dataPtr; }
    constexpr const_pointer data() const { return dataPtr + offset; }
    constexpr const_pointer ptr() const { return dataPtr; }

    constexpr reference front() { return *data(); }
    constexpr reference back() { return *(data() + (length - 1)); }
    constexpr const_reference front() const { return *data(); }
    constexpr const_reference back() const { return *(data() + (length - 1)); }

    constexpr void reset()
    {
        offset = 0;
        length = 0;
        dataPtr = nullptr;
    }

    const_reference operator[] (SizeT idx) const { return dataPtr[offset + idx]; }
    reference operator[] (SizeT idx) { return dataPtr[offset + idx]; }

    NODISCARD iterator begin() noexcept { return iterator(dataPtr + offset, 0); }
    NODISCARD iterator end() noexcept { return iterator(dataPtr + offset, length); }
    NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    NODISCARD const_iterator cbegin() const noexcept { return const_iterator(dataPtr + offset, 0); }
    NODISCARD const_iterator cend() const noexcept { return const_iterator(dataPtr + offset, length); }
    NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }
};