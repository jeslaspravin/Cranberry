#pragma once

namespace HashUtility
{
    template <typename T>
    inline void hashCombine(size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    inline void hashCombine(size_t& seed1, const size_t& seed2)
    {
        seed1 ^= seed2 + 0x9e3779b9 + (seed1 << 6) + (seed1 >> 2);
    }

    template <typename It>
    inline size_t hashRange(It first,It last)
    {
        std::hash<decltype(*first)> hasher;
        size_t seed = 0;
        for (; first != last; ++first)
        {
            hashCombine(seed, hasher(*first));
        }
        return seed;
    }
}