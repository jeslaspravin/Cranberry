/*!
 * \file BitArray.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"

template <typename BitArrayType, bool bIsConst>
class BitArrayIterator;

template <typename BitArrayType>
class BitReference
{
public:
    using value_type = typename BitArrayType::value_type;
    using BitIdxType = typename BitArrayType::BitIdxType;

    friend BitArrayIterator<BitArrayType, false>;

private:
    value_type *bitElement = nullptr;
    value_type bitMask;

public:
    CONST_EXPR BitReference() = default;
    CONST_EXPR BitReference(value_type *element, value_type mask)
        : bitElement(element)
        , bitMask(mask)
    {}

    CONST_EXPR operator bool () const noexcept { return bitElement ? BIT_SET(*bitElement, bitMask) : false; }

    CONST_EXPR BitReference &operator= (bool bValue) noexcept
    {
        if (bitElement)
        {
            bValue ? SET_BITS(*bitElement, bitMask) : CLEAR_BITS(*bitElement, bitMask);
        }
        return *this;
    }

    CONST_EXPR bool operator!= (const BitReference &other) const noexcept
    {
        return bitElement != other->bitElement || bitMask != other->bitMask;
    }
};

template <std::unsigned_integral ElementType>
class BitArray
{
public:
    using value_type = ElementType;
    using const_reference = bool;
    using reference = BitReference<BitArray>;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using ArraySizeType = SizeT;
    using ArrayDiffType = SSizeT;
    // Index can be uint8 however Bit shifting causes issues by rotating the bit around after 7th bit
    // I could avoid it by manually casting when bit shifting, However BitIdxType is not stored anywhere except in Iterators so uint64 is fine
    using BitIdxType = uint64;
    using BitIdxDiffType = int64;
    using BitVectorType = std::vector<value_type>;

    template <bool bIsConst>
    using IteratorType = BitArrayIterator<BitArray, bIsConst>;

    friend BitArrayIterator<BitArray, true>;
    friend BitArrayIterator<BitArray, false>;

    using iterator = IteratorType<false>;
    using const_iterator = IteratorType<true>;
    using reverse_iterator = std::reverse_iterator<IteratorType<false>>;
    using const_reverse_iterator = std::reverse_iterator<IteratorType<true>>;

    CONST_EXPR static BitIdxType unsignedLog2(BitIdxType value)
    {
        BitIdxType power = 0;
        while (BIT_NOT_SET(value, 1))
        {
            ++power;
            value >>= 1;
        }
        return power;
    }

    CONST_EXPR static const value_type ALL_BITS_SET = ~((value_type)0);
    CONST_EXPR static const BitIdxType BITS_PER_ELEMENT = sizeof(value_type) * 8;
    CONST_EXPR static const value_type BITS_IDX_MASK = (BITS_PER_ELEMENT - 1);
    CONST_EXPR static const value_type ARRAY_IDX_MASK = ~BITS_IDX_MASK;
    CONST_EXPR static const BitIdxType ARRAY_IDX_SHIFT = unsignedLog2(BITS_PER_ELEMENT);

private:
    SizeT bitsCount = 0;
    BitVectorType bits;

public:
    BitArray() = default;
    BitArray(const BitArray &) = default;
    BitArray(BitArray &&) = default;
    BitArray &operator= (const BitArray &) = default;
    BitArray &operator= (BitArray &&) = default;

    CONST_EXPR BitArray(SizeT initialSize) noexcept
        : bitsCount(initialSize)
        , bits(arraySizeForBits(initialSize), 0)
    {}
    CONST_EXPR BitArray(std::initializer_list<value_type> initList) noexcept
        : bitsCount(initList.size() * BITS_PER_ELEMENT)
        , bits(initList)
    {}
    CONST_EXPR BitArray(std::initializer_list<bool> initList) noexcept { this->operator= (initList); }
    CONST_EXPR BitArray &operator= (std::initializer_list<value_type> initList) noexcept
    {
        bitsCount = initList.size() * BITS_PER_ELEMENT;
        bits = initList;
        return (*this);
    }
    CONST_EXPR BitArray &operator= (std::initializer_list<bool> initList) noexcept
    {
        bitsCount = initList.size();
        bits.resize(arraySizeForBits(initList.size()), 0);
        ArraySizeType arrayIdx = 0;
        BitIdxType elementBitsCount = Math::min((BitIdxType)initList.size(), BITS_PER_ELEMENT);
        while (true)
        {
            value_type bitMask = 1;
            // Setting each init list bool to a bit
            for (BitIdxType bitIdx = 0; bitIdx < elementBitsCount; ++bitIdx)
            {
                SizeT initValIdx = arrayIdxToBitIdx(arrayIdx, bitIdx);
                SET_BITS(bits[arrayIdx], initList.begin()[initValIdx] ? bitMask : 0);
                bitMask <<= 1;
            }
            ++arrayIdx;
            // Find new bits count for next bit array element
            SizeT initValIdx = arrayIdxToBitIdx(arrayIdx, 0);
            if (initValIdx >= initList.size())
            {
                // We are past initializer list counts
                break;
            }
            elementBitsCount = Math::min((BitIdxType)(initList.size() - initValIdx), BITS_PER_ELEMENT);
        }
        return (*this);
    }

    // Element access
    CONST_EXPR const_reference operator[] (SizeT bitIdx) const noexcept
    {
        debugAssert(bitIdx < bitsCount);

        BitIdxType bitOffset;
        ArraySizeType arrayIdx = bitIdxToArrayIdx(bitOffset, bitIdx);
        return BIT_SET(bits[arrayIdx], INDEX_TO_FLAG_MASK(bitOffset));
    }
    CONST_EXPR reference operator[] (SizeT bitIdx) noexcept
    {
        debugAssert(bitIdx < bitsCount);

        BitIdxType bitOffset;
        ArraySizeType arrayIdx = bitIdxToArrayIdx(bitOffset, bitIdx);
        return reference(&bits[arrayIdx], INDEX_TO_FLAG_MASK(bitOffset));
    }
    CONST_EXPR const_reference at(SizeT bitIdx) const noexcept
    {
        fatalAssertf(bitIdx < bitsCount, "Accessing out of index {}", bitIdx);
        return this->operator[] (bitIdx);
    }
    CONST_EXPR const_reference front() const noexcept
    {
        if (empty())
        {
            return false;
        }

        return BIT_SET(bits[0], INDEX_TO_FLAG_MASK(0));
    }
    CONST_EXPR reference front(SizeT bitIdx) noexcept
    {
        if (empty())
        {
            return reference();
        }

        return reference(&bits[0], INDEX_TO_FLAG_MASK(0));
    }
    CONST_EXPR const_reference back() const noexcept
    {
        if (size() == 0)
        {
            return false;
        }

        return this->operator[] (bitsCount - 1);
    }
    CONST_EXPR reference back(SizeT bitIdx) noexcept
    {
        if (empty())
        {
            return reference();
        }

        return this->operator[] (bitsCount - 1);
    }
    CONST_EXPR pointer data() noexcept { return bits.data(); }
    CONST_EXPR const_pointer data() const noexcept { return bits.data(); }

    // Capacity
    NODISCARD CONST_EXPR FORCE_INLINE bool empty() const { return bitsCount == 0; }
    CONST_EXPR FORCE_INLINE SizeT size() const { return bitsCount; }
    CONST_EXPR FORCE_INLINE SizeT capacity() const { return bits.capacity() * BITS_PER_ELEMENT; }
    CONST_EXPR SizeT max_size() const { return bits.max_size(); }
    CONST_EXPR void reserve(SizeT newCap) noexcept { bits.reserve(arraySizeForBits(newCap)); }
    CONST_EXPR void shrink_to_fit() { bits.shrink_to_fit(); }

    // Modifiers
    CONST_EXPR void clear() noexcept
    {
        bitsCount = 0;
        bits.clear();
    }
    CONST_EXPR void resize(SizeT newSize, value_type newVal = 0)
    {
        if (newSize > bitsCount)
        {
            bits.resize(arraySizeForBits(newSize), newVal);
        }
        else
        {
            // Clear extra bits
            for (ArraySizeType idx = arraySizeForBits(newSize); idx < bits.size(); ++idx)
            {
                bits[idx] = 0;
            }
        }
        bitsCount = newSize;
    }
    CONST_EXPR void swap(BitArray &other) noexcept
    {
        std::swap(bitsCount, other.bitsCount);
        bits.swap(other.bits);
    }

    CONST_EXPR void emplace_back(value_type value)
    {
        SizeT oldBitsCount = bitsCount;
        resize(bitsCount + BITS_PER_ELEMENT);

        if (value > 0)
        {
            BitIdxType bitStartIdx;
            ArraySizeType arrayIdx = bitIdxToArrayIdx(bitStartIdx, oldBitsCount);
            // If starting at boundary of bit storage element then we can set directly
            if (bitStartIdx == 0)
            {
                bits[arrayIdx] = value;
            }
            else
            {
                value_type oldValueMask = INDEX_TO_FLAG_MASK(bitStartIdx) - 1;
                // shift left to remove number of bits that does not fits in this array element
                REPLACE_BITS_MASKED(bits[arrayIdx], value << bitStartIdx, ~oldValueMask);
                // shift right by bits set to previous element to remove number of bits that are
                // already added and old value mask is all bits left to add
                REPLACE_BITS_MASKED(bits[arrayIdx + 1], value >> (BITS_PER_ELEMENT - bitStartIdx), oldValueMask);
            }
        }
    }

    CONST_EXPR void push_back(value_type value) { emplace_back(value); }

    CONST_EXPR void pop_back() noexcept
    {
        if (bitsCount == 0)
        {
            return;
        }

        --bitsCount;

        BitIdxType bitOffset;
        ArraySizeType arrayIdx = bitIdxToArrayIdx(bitOffset, bitsCount);
        CLEAR_BIT_AT(bits[arrayIdx], bitOffset);
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

    // Additional functions
    CONST_EXPR void append(std::initializer_list<value_type> initList)
    {
        if (initList.size() == 0)
        {
            return;
        }

        BitIdxType bitStartIdx;
        ArraySizeType arrayIdx = bitIdxToArrayIdx(bitStartIdx, bitsCount);

        // Resizing here to avoid resize every time we do individual emplace
        bits.resize(bits.size() + initList.size());

        // We can do direct inserts
        if (bitStartIdx == 0)
        {
            bits.insert(bits.end(), initList);
        }
        else
        {
            for (value_type *val : initList)
            {
                emplace_back(*val);
            }
        }
    }
    CONST_EXPR void append(std::initializer_list<bool> initList)
    {
        if (initList.size() == 0)
        {
            return;
        }

        SizeT oldBitsCount = bitsCount;
        resize(oldBitsCount + initList.size());

        for (SizeT bitOffset = 0; bitOffset < initList.size(); ++bitOffset)
        {
            (*this)[oldBitsCount + bitOffset] = initList.begin()[bitOffset];
        }
    }

    CONST_EXPR void add(SizeT count, value_type newVal = 0) { resize(bitsCount + count, newVal); }
    CONST_EXPR void setRange(SizeT offset, SizeT count)
    {
        if (count == 0)
        {
            return;
        }

        // <= as end is exclusive
        debugAssert(offset + count <= bitsCount);

        BitIdxType startBitIdx;
        ArraySizeType startArrayIdx = bitIdxToArrayIdx(startBitIdx, offset);

        BitIdxType endBitIdx;
        ArraySizeType endArrayIdx = bitIdxToArrayIdx(endBitIdx, offset + count);

        // Range is within single value
        if (startArrayIdx == endArrayIdx)
        {
            // If starting from idx 3 and ending at 6 then (0b11111000 & 0b00111111) = 0b00111000
            const value_type startValueMask = ~(INDEX_TO_FLAG_MASK(startBitIdx) - 1);
            const value_type endValueMask = INDEX_TO_FLAG_MASK(endBitIdx) - 1;
            SET_BITS(bits[endArrayIdx], startValueMask & endValueMask);
            return;
        }

        if (startBitIdx != 0)
        {
            // If starting from idx 3 then ~(0b001000 - 1) = ~(0b00111) = 0b11000 bits must be set
            const value_type startValueMask = ~(INDEX_TO_FLAG_MASK(startBitIdx) - 1);
            SET_BITS(bits[startArrayIdx], startValueMask);
            startArrayIdx++;
            startBitIdx = 0;
        }
        // End array index is aligned if exclusive last bit idx is at 0 of exclusive end bit array idx
        if (endBitIdx != 0)
        {
            // If ending before idx 3 then 0b00100 - 1 = 0b00011 bits must be set
            const value_type endValueMask = INDEX_TO_FLAG_MASK(endBitIdx) - 1;
            SET_BITS(bits[endArrayIdx], endValueMask);
            // No need to decrement endArrayIdx as it will be exclusive
            endBitIdx = 0;
        }

        debugAssert(startBitIdx == 0 && endBitIdx == 0);
        for (ArraySizeType arrayIdx = startArrayIdx; arrayIdx != endArrayIdx; ++arrayIdx)
        {
            bits[arrayIdx] = ALL_BITS_SET;
        }
    }
    CONST_EXPR void resetRange(SizeT offset, SizeT count)
    {
        if (count == 0)
        {
            return;
        }

        // <= as end is exclusive
        debugAssert(offset + count <= bitsCount);

        BitIdxType startBitIdx;
        ArraySizeType startArrayIdx = bitIdxToArrayIdx(startBitIdx, offset);

        BitIdxType endBitIdx;
        ArraySizeType endArrayIdx = bitIdxToArrayIdx(endBitIdx, offset + count);

        // Range is within single value
        if (startArrayIdx == endArrayIdx)
        {
            // If starting from idx 3 and ending at 6 then (0b11111000 & 0b00111111) = 0b00111000
            const value_type startValueMask = ~(INDEX_TO_FLAG_MASK(startBitIdx) - 1);
            const value_type endValueMask = INDEX_TO_FLAG_MASK(endBitIdx) - 1;
            CLEAR_BITS(bits[endArrayIdx], startValueMask & endValueMask);
            return;
        }

        if (startBitIdx != 0)
        {
            // If starting from idx 3 then ~(0b001000 - 1) = ~(0b00111) = 0b11000 bits must be set
            const value_type startValueMask = ~(INDEX_TO_FLAG_MASK(startBitIdx) - 1);
            CLEAR_BITS(bits[startArrayIdx], startValueMask);
            startArrayIdx++;
            startBitIdx = 0;
        }
        // End array index is aligned if exclusive last bit idx is at 0 of exclusive end bit array idx
        if (endBitIdx != 0)
        {
            // If ending before idx 3 then 0b00100 - 1 = 0b00011 bits must be reset
            const value_type endValueMask = INDEX_TO_FLAG_MASK(endBitIdx) - 1;
            CLEAR_BITS(bits[endArrayIdx], endValueMask);
            // No need to decrement endArrayIdx as it will be exclusive
            endBitIdx = 0;
        }

        debugAssert(startBitIdx == 0 && endBitIdx == 0);
        for (ArraySizeType arrayIdx = startArrayIdx; arrayIdx < endArrayIdx; ++arrayIdx)
        {
            bits[arrayIdx] = 0;
        }
    }
    CONST_EXPR bool checkRange(SizeT offset, SizeT count, bool bCheckIfSet) const
    {
        if (count == 0)
        {
            return true;
        }

        // <= as end is exclusive
        debugAssert(offset + count <= bitsCount);

        BitIdxType startBitIdx;
        ArraySizeType startArrayIdx = bitIdxToArrayIdx(startBitIdx, offset);

        BitIdxType endBitIdx;
        ArraySizeType endArrayIdx = bitIdxToArrayIdx(endBitIdx, offset + count);

        // Range is within single value
        if (startArrayIdx == endArrayIdx)
        {
            // If starting from idx 3 and ending at 6 then (0b11111000 & 0b00111111) = 0b00111000
            const value_type startValueMask = ~(INDEX_TO_FLAG_MASK(startBitIdx) - 1);
            const value_type endValueMask = INDEX_TO_FLAG_MASK(endBitIdx) - 1;
            const value_type valueMask = startValueMask & endValueMask;
            return bCheckIfSet ? BIT_SET(bits[endArrayIdx], valueMask) : !ANY_BIT_SET(bits[endArrayIdx], valueMask);
        }

        bool bRetVal = true;
        if (startBitIdx != 0)
        {
            // If starting from idx 3 then ~(0b001000 - 1) = ~(0b00111) = 0b11000 bits must be set
            const value_type startValueMask = ~(INDEX_TO_FLAG_MASK(startBitIdx) - 1);
            bRetVal
                = bRetVal && (bCheckIfSet ? BIT_SET(bits[startArrayIdx], startValueMask) : !ANY_BIT_SET(bits[startArrayIdx], startValueMask));
            startArrayIdx++;
            startBitIdx = 0;
        }
        // End array index is aligned if exclusive last bit idx is at 0 of exclusive end bit array idx
        if (endBitIdx != 0)
        {
            // If ending before idx 3 then 0b00100 - 1 = 0b00011 bits must be reset
            const value_type endValueMask = INDEX_TO_FLAG_MASK(endBitIdx) - 1;
            bRetVal = bRetVal && (bCheckIfSet ? BIT_SET(bits[endArrayIdx], endValueMask) : !ANY_BIT_SET(bits[endArrayIdx], endValueMask));
            // No need to decrement endArrayIdx as it will be exclusive
            endBitIdx = 0;
        }

        debugAssert(startBitIdx == 0 && endBitIdx == 0);
        for (ArraySizeType arrayIdx = startArrayIdx; arrayIdx < endArrayIdx; ++arrayIdx)
        {
            bRetVal = bRetVal && (bCheckIfSet ? bits[arrayIdx] == ALL_BITS_SET : bits[arrayIdx] == 0);
        }
        return bRetVal;
    }

    // Counts bits that are set
    SizeT countOnes() const
    {
        SizeT count = 0;
        SizeT fullElemCount = bitsCount / BITS_PER_ELEMENT;
        for (SizeT i = 0; i < fullElemCount; ++i)
        {
            count += PlatformFunctions::getSetBitCount(bits[i]);
        }
        if (fullElemCount < bits.size())
        {
            debugAssert((bits.size() - fullElemCount) == 1);
            SizeT bitsLeft = bitsCount - (fullElemCount * BITS_PER_ELEMENT);
            // If 3 bits left the mask must be (0b1000 - 1) = 0b0111
            SizeT mask = INDEX_TO_FLAG_MASK(bitsLeft) - 1;
            count += PlatformFunctions::getSetBitCount(bits[fullElemCount] & mask);
        }
        return count;
    }
    // Counts bits that are not set
    SizeT countZeroes() const { return bitsCount - countOnes(); }

private:
    // Returns array index from bit idx and sets the bit idx within this element
    CONST_EXPR static ArraySizeType bitIdxToArrayIdx(BitIdxType &outBitIdx, SizeT bitIdx)
    {
        ArraySizeType retVal = (bitIdx & ARRAY_IDX_MASK) >> ARRAY_IDX_SHIFT;
        outBitIdx = (BitIdxType)(bitIdx & BITS_IDX_MASK);
        return retVal;
    }
    // Unsigned version where both arrayIdx and bitIdx are valid indices
    CONST_EXPR static SizeT arrayIdxToBitIdx(ArraySizeType arrayIdx, uint8 bitIdx) { return (arrayIdx << ARRAY_IDX_SHIFT) + bitIdx; }
    // Signed version where arrayIdx or bitIdx is negative difference
    CONST_EXPR static ArrayDiffType arrayIdxToBitIdx(ArrayDiffType arrayDiff, BitIdxDiffType bitIdxDiff)
    {
        return (arrayDiff * BITS_PER_ELEMENT) + bitIdxDiff;
    }
    CONST_EXPR static ArraySizeType arraySizeForBits(SizeT bitsCount)
    {
        BitIdxType bitIdx;
        ArraySizeType arrayCount = bitIdxToArrayIdx(bitIdx, bitsCount);
        arrayCount += (bitIdx > 0);
        return arrayCount;
    }
};

template <typename BitArrayType, bool bIsConst>
class BitArrayIteratorTrait;

template <typename BitArrayType>
class BitArrayIteratorTrait<BitArrayType, true>
{
public:
    using value_type = bool;
    using reference = value_type;
    using const_reference = value_type;
    using pointer = value_type;
    using const_pointer = value_type;
    using difference_type = SSizeT;
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;

    using BitVectorItr = typename BitArrayType::BitVectorType::const_iterator;
    using BitIdxType = typename BitArrayType::BitIdxType;

    // Constructs value from params
    CONST_EXPR static value_type getValue(BitVectorItr bitElemItr, BitIdxType bitRefIdx)
    {
        return BIT_SET(*bitElemItr, INDEX_TO_FLAG_MASK(bitRefIdx));
    }
};

template <typename BitArrayType>
class BitArrayIteratorTrait<BitArrayType, false>
{
public:
    using value_type = BitReference<BitArrayType>;
    using reference = value_type;
    using const_reference = value_type;
    using pointer = value_type;
    using const_pointer = value_type;
    using difference_type = SSizeT;
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;

    using BitVectorItr = typename BitArrayType::BitVectorType::iterator;
    using BitIdxType = typename BitArrayType::BitIdxType;

    // Constructs value from params
    CONST_EXPR static value_type getValue(BitVectorItr bitElemItr, BitIdxType bitRefIdx)
    {
        return value_type(&*bitElemItr, INDEX_TO_FLAG_MASK(bitRefIdx));
    }
};

// Iterator invalidates for all vector iterator invalidation cases
template <typename BitArrayType, bool bIsConst>
class BitArrayIterator
{
public:
    using Traits = BitArrayIteratorTrait<BitArrayType, bIsConst>;

    using BitIdxType = typename BitArrayType::BitIdxType;
    using BitIdxDiffType = typename BitArrayType::BitIdxDiffType;
    using ArraySizeType = typename BitArrayType::ArraySizeType;
    using ArrayDiffType = typename BitArrayType::ArrayDiffType;

    using BitVectorItr = typename Traits::BitVectorItr;

    using value_type = typename Traits::value_type;
    using reference = typename Traits::reference;
    using const_reference = typename Traits::const_reference;
    using pointer = typename Traits::pointer;
    using const_pointer = typename Traits::const_pointer;
    using difference_type = typename Traits::difference_type;
    using iterator_concept = typename Traits::iterator_concept;
    using iterator_category = typename Traits::iterator_category;

private:
    BitVectorItr bitElemItr;
    BitIdxType bitRefIdx;

private:
    CONST_EXPR void addOffset(const difference_type off)
    {
        BitIdxType bitIdx;
        ArraySizeType arrayCount = BitArrayType::bitIdxToArrayIdx(bitIdx, bitRefIdx + off);
        bitRefIdx = bitIdx;
        bitElemItr += arrayCount;
    }
    CONST_EXPR void subOffset(const difference_type off)
    {
        // Since off will always be negative
        SizeT absOff = -off;
        if (bitRefIdx < absOff)
        {
            // Offset by current bitRefIdx first
            absOff -= bitRefIdx;
            --bitElemItr;
            // Number of bits to offset after arrayIdxOffset
            BitIdxType bitIdxOff;
            ArraySizeType arrayIdxOff = BitArrayType::bitIdxToArrayIdx(bitIdxOff, absOff);

            bitElemItr -= arrayIdxOff;
            bitRefIdx = BitArrayType::BITS_PER_ELEMENT - 1 - bitIdxOff;
        }
        else
        {
            bitRefIdx -= absOff;
        }
    }

public:
    CONST_EXPR BitArrayIterator() = delete;
    CONST_EXPR BitArrayIterator(const BitArrayIterator &) = default;
    CONST_EXPR BitArrayIterator(BitArrayIterator &&) = default;
    CONST_EXPR BitArrayIterator &operator= (const BitArrayIterator &) = default;
    CONST_EXPR BitArrayIterator &operator= (BitArrayIterator &&) = default;
    CONST_EXPR BitArrayIterator(BitVectorItr itr, BitIdxType bitIdx)
        : bitElemItr(itr)
        , bitRefIdx(bitIdx)
    {}

    CONST_EXPR pointer operator->() noexcept { return Traits::getValue(bitElemItr, bitRefIdx); }
    NODISCARD CONST_EXPR reference operator* () noexcept { return Traits::getValue(bitElemItr, bitRefIdx); }
    CONST_EXPR const_pointer operator->() const noexcept { return Traits::getValue(bitElemItr, bitRefIdx); }
    NODISCARD CONST_EXPR const_reference operator* () const noexcept { return Traits::getValue(bitElemItr, bitRefIdx); }

    CONST_EXPR bool operator!= (const BitArrayIterator &other) const noexcept
    {
        return bitElemItr != other.bitElemItr || bitRefIdx != other.bitRefIdx;
    }

    CONST_EXPR BitArrayIterator &operator++ () noexcept
    {
        addOffset(1);
        return *this;
    }

    NODISCARD CONST_EXPR BitArrayIterator operator++ (int) noexcept
    {
        BitArrayIterator retVal(*this);
        this->operator++ ();
        return retVal;
    }

    CONST_EXPR BitArrayIterator &operator-- () noexcept
    {
        if (bitRefIdx == 0)
        {
            bitRefIdx = BitArrayType::BITS_PER_ELEMENT - 1;
            --bitElemItr;
        }
        else
        {
            --bitRefIdx;
        }
        return *this;
    }

    NODISCARD CONST_EXPR BitArrayIterator operator-- (int) noexcept
    {
        BitArrayIterator retVal(*this);
        this->operator-- ();
        return retVal;
    }

    CONST_EXPR BitArrayIterator &operator+= (const difference_type off) noexcept
    {
        if (off < 0)
        {
            subOffset(off);
        }
        else
        {
            addOffset(off);
        }
        return *this;
    }

    NODISCARD CONST_EXPR BitArrayIterator operator+ (const difference_type off) const noexcept
    {
        BitArrayIterator retVal(*this);
        retVal += off;
        return retVal;
    }

    CONST_EXPR BitArrayIterator &operator-= (const difference_type off) noexcept
    {
        (*this) += (-off);
        return *this;
    }

    NODISCARD CONST_EXPR BitArrayIterator operator- (const difference_type off) const noexcept
    {
        BitArrayIterator retVal(*this);
        retVal -= off;
        return retVal;
    }
    NODISCARD CONST_EXPR difference_type operator- (const BitArrayIterator &other) const noexcept;

    NODISCARD CONST_EXPR value_type operator[] (const difference_type off) const noexcept
    {
        BitArrayIterator retVal(*this);
        retVal += off;
        return *retVal;
    }
};

template <std::unsigned_integral ElementType>
NODISCARD CONST_EXPR typename BitArray<ElementType>::iterator BitArray<ElementType>::begin() noexcept
{
    return iterator(bits.begin(), 0);
}

template <std::unsigned_integral ElementType>
NODISCARD CONST_EXPR typename BitArray<ElementType>::const_iterator BitArray<ElementType>::begin() const noexcept
{
    return const_iterator(bits.cbegin(), 0);
}

template <std::unsigned_integral ElementType>
NODISCARD CONST_EXPR typename BitArray<ElementType>::iterator BitArray<ElementType>::end() noexcept
{
    BitIdxType bitOffset;
    ArraySizeType arrayIdx = bitIdxToArrayIdx(bitOffset, bitsCount);
    return iterator(bits.begin() + arrayIdx, bitOffset);
}

template <std::unsigned_integral ElementType>
NODISCARD CONST_EXPR typename BitArray<ElementType>::const_iterator BitArray<ElementType>::end() const noexcept
{
    BitIdxType bitOffset;
    ArraySizeType arrayIdx = bitIdxToArrayIdx(bitOffset, bitsCount);
    return const_iterator(bits.cbegin() + arrayIdx, bitOffset);
}

template <typename BitArrayType, bool bIsConst>
NODISCARD CONST_EXPR typename BitArrayIterator<BitArrayType, bIsConst>::difference_type
BitArrayIterator<BitArrayType, bIsConst>::operator- (const BitArrayIterator &other) const noexcept
{
    ArrayDiffType arrayIdxDiff = bitElemItr - other.bitElemItr;
    BitIdxDiffType bitDiff = bitRefIdx - other.bitRefIdx;
    return BitArrayType::arrayIdxToBitIdx(arrayIdxDiff, bitDiff);
}

// SparseVector policy
class BitArraySparsityPolicy
{
public:
    using ValueType = dword;
    using SparsityContainerType = BitArray<ValueType>;
    using SizeType = typename SparsityContainerType::ArraySizeType;

    using value_type = ValueType;
    using size_type = SizeType;

    SparsityContainerType sparsityTags;

public:
    CONST_EXPR void clear() { sparsityTags.clear(); }
    CONST_EXPR void reserve(SizeType count) noexcept { sparsityTags.reserve(count); }
    CONST_EXPR void resize(SizeType count, bool bSet = 0)
    {
        const ValueType value = bSet ? ~(ValueType(0)) : 0;
        sparsityTags.resize(count, value);
    }
    // Sets the value at idx to be occupied
    CONST_EXPR void set(SizeType idx) { sparsityTags[idx] = true; }
    // Sets the value at idx to be free
    CONST_EXPR void reset(SizeType idx) { sparsityTags[idx] = false; }
    SizeT pop_free()
    {
        SizeT idx = 0;
        for (bool bIsOccupied : sparsityTags)
        {
            if (!bIsOccupied)
            {
                set(idx);
                return idx;
            }
            ++idx;
        }
        debugAssert(!"No free index available");
        return idx;
    }
    CONST_EXPR void push_free(SizeType idx)
    {
        if (sparsityTags.size() > idx)
        {
            reset(idx);
        }
        else
        {
            resize(idx + 1);
        }
    }
    CONST_EXPR bool isFree(SizeType idx) const { return !sparsityTags[idx]; }
    // true if not free slots found
    NODISCARD FORCE_INLINE bool empty() const { return sparsityTags.countZeroes() == 0; }
    // Number of sparse slots
    FORCE_INLINE SizeType size() const { return sparsityTags.countZeroes(); }
};
