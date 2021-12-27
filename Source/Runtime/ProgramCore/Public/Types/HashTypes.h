#pragma once
#include "Types/CoreDefines.h"
#include "Types/Traits/TypeTraits.h"

template <class T>
concept SeedType = std::is_same_v<std::remove_cvref_t<T>, size_t>;

namespace HashUtility
{
    template <typename T, typename CleanType = std::remove_cvref_t<T>>
    CONST_EXPR size_t hash(T&& v)
    {
        std::hash<CleanType> hasher;
        return hasher(std::forward<T>(v));
    }

    template <typename T, typename CleanType = std::remove_cvref_t<T>>
    CONST_EXPR void hashCombine(size_t& seed, T&& v)
    {
        std::hash<CleanType> hasher;
        seed ^= hasher(std::forward<T>(v)) 
            + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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
    void combineSeeds(size_t& outSeed, SeedTypes&&... seeds);
    template <>
    FORCE_INLINE void combineSeeds(size_t& outSeed)
    {}
    template <typename FirstSeedType, typename... SeedTypes>
    FORCE_INLINE void combineSeeds(size_t& outSeed, FirstSeedType&& seed, SeedTypes&&... seeds)
    {
        outSeed ^= seed + 0x9e3779b9 + (outSeed << 6) + (outSeed >> 2);
        combineSeeds(outSeed, std::forward<SeedTypes>(seeds)...);
    }

    // Below impl creates a lot of seed and has huge stack size, using void hashAll(size_t& outSeed, Types&&... hashable) is recommended
    template <typename... Types>
    DEBUG_INLINE size_t hashAllReturn(Types&&... hashables)
    {
        size_t seed = 0;
        combineSeeds(seed, HashUtility::hash<Types>(std::forward<Types>(hashables))...);
        return seed;
    }

    template <typename... Types>
    void hashAllInto(size_t& outSeed, Types&&... hashables);
    template <>
    FORCE_INLINE void hashAllInto(size_t& outSeed)
    {}
    template <typename FirstType, typename... Types>
    FORCE_INLINE void hashAllInto(size_t& outSeed, FirstType&& hashable, Types&&... hashables)
    {
        HashUtility::hashCombine<FirstType>(outSeed, std::forward<FirstType>(hashable));
        hashAllInto(outSeed, std::forward<Types>(hashables)...);
    }
}


template <typename FirstType, typename SecondType>
struct PROGRAMCORE_EXPORT std::hash<std::pair<FirstType, SecondType>>
{
    NODISCARD size_t operator()(const std::pair<FirstType, SecondType>& val) const noexcept
    {
        auto hashCode = HashUtility::hash(val.first);
        HashUtility::hashCombine(hashCode, val.second);
        return hashCode;
    }
};