/*!
 * \file HashTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "Types/CoreDefines.h"
#include "Types/CompilerDefines.h"
#include "Types/Templates/TypeTraits.h"

#include <utility>

template <class T>
concept SeedType = std::is_same_v<std::remove_cvref_t<T>, size_t>;

namespace HashUtility
{
// Removes any const ref, Type & const -> Type
template <typename T, typename CleanType = std::remove_cvref_t<T>>
CONST_EXPR size_t hash(T &&v)
{
    std::hash<CleanType> hasher;
    return hasher(std::forward<T>(v));
}

// Pointer hash
template <typename T>
CMATH_CONSTEXPR size_t hash(const T *val)
{
    // https://stackoverflow.com/a/21062236/18816213
    CMATH_CONSTEXPR static const size_t shift = (size_t)log2(1 + sizeof(T));
    return (size_t)(val) >> shift;
}

template <typename T, typename CleanType = std::remove_cvref_t<T>>
CONST_EXPR void hashCombine(size_t &seed, T &&v)
{
    std::hash<CleanType> hasher;
    seed ^= hasher(std::forward<T>(v)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename It>
DEBUG_INLINE size_t hashRange(It first, It last)
{
    std::hash<decltype(*first)> hasher;
    size_t seed = 0;
    for (; first != last; ++first)
    {
        hashCombine(seed, hasher(*first));
    }
    return seed;
}

template <typename... SeedTypes>
void combineSeeds(size_t &outSeed, SeedTypes &&...seeds);
template <>
FORCE_INLINE void combineSeeds(size_t &/*outSeed*/)
{}
template <typename FirstSeedType, typename... SeedTypes>
FORCE_INLINE void combineSeeds(size_t &outSeed, FirstSeedType &&seed, SeedTypes &&...seeds)
{
    outSeed ^= seed + 0x9e3779b9 + (outSeed << 6) + (outSeed >> 2);
    combineSeeds(outSeed, std::forward<SeedTypes>(seeds)...);
}

// Below impl creates a lot of seed and has huge stack size, using void hashAll(size_t& outSeed,
// Types&&... hashable) is recommended
template <typename... Types>
DEBUG_INLINE size_t hashAllReturn(Types &&...hashables)
{
    size_t seed = 0;
    combineSeeds(seed, HashUtility::hash<Types>(std::forward<Types>(hashables))...);
    return seed;
}

template <typename... Types>
void hashAllInto(size_t &outSeed, Types &&...hashables);
template <>
FORCE_INLINE void hashAllInto(size_t &/*outSeed*/)
{}
template <typename FirstType, typename... Types>
FORCE_INLINE void hashAllInto(size_t &outSeed, FirstType &&hashable, Types &&...hashables)
{
    HashUtility::hashCombine<FirstType>(outSeed, std::forward<FirstType>(hashable));
    hashAllInto(outSeed, std::forward<Types>(hashables)...);
}
} // namespace HashUtility

template <typename FirstType, typename SecondType>
struct std::hash<std::pair<FirstType, SecondType>>
{
    NODISCARD size_t operator()(const std::pair<FirstType, SecondType> &val) const noexcept
    {
        auto hashCode = HashUtility::hash(val.first);
        HashUtility::hashCombine(hashCode, val.second);
        return hashCode;
    }
};

// For Transparent hash containers, equality and comparer.
template <typename PtrType>
struct std::hash<PtrType *>
{
    using is_transparent = std::true_type;
    using ConstPtr = std::remove_cv_t<PtrType> const *;

    NODISCARD size_t operator()(ConstPtr const &ptr) const noexcept { return HashUtility::hash(ptr); }
};
template <typename PtrType>
struct std::equal_to<PtrType *>
{
    using is_transparent = std::true_type;
    using ConstPtr = std::remove_cv_t<PtrType> const *;

    NODISCARD constexpr bool operator()(ConstPtr const &lhs, ConstPtr const &rhs) const { return lhs == rhs; }
    NODISCARD constexpr bool operator()(UPtrInt lhs, ConstPtr const &rhs) const { return reinterpret_cast<ConstPtr>(lhs) == rhs; }
    NODISCARD constexpr bool operator()(ConstPtr const &lhs, UPtrInt rhs) const { return lhs == reinterpret_cast<ConstPtr>(rhs); }
};
template <typename PtrType>
struct std::less<PtrType *>
{
    using is_transparent = std::true_type;
    using ConstPtr = std::remove_cv_t<PtrType> const *;

    NODISCARD constexpr bool operator()(ConstPtr const &lhs, ConstPtr const &rhs) const { return lhs < rhs; }
    NODISCARD constexpr bool operator()(UPtrInt lhs, ConstPtr const &rhs) const { return reinterpret_cast<ConstPtr>(lhs) < rhs; }
    NODISCARD constexpr bool operator()(ConstPtr const &lhs, UPtrInt rhs) const { return lhs < reinterpret_cast<ConstPtr>(rhs); }
};
template <typename PtrType>
struct std::greater<PtrType *>
{
    using is_transparent = std::true_type;
    using ConstPtr = std::remove_cv_t<PtrType> const *;

    NODISCARD constexpr bool operator()(ConstPtr const &lhs, ConstPtr const &rhs) const { return lhs > rhs; }
    NODISCARD constexpr bool operator()(UPtrInt lhs, ConstPtr const &rhs) const { return reinterpret_cast<ConstPtr>(lhs) > rhs; }
    NODISCARD constexpr bool operator()(ConstPtr const &lhs, UPtrInt rhs) const { return lhs > reinterpret_cast<ConstPtr>(rhs); }
};