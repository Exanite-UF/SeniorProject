#pragma once

#include <functional>
#include <tuple>

template <typename TT>
struct TupleHasher;

// Based on https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
namespace
{
    // Code from boost
    // Reciprocal of the golden ratio helps spread entropy and handles duplicates.
    // See Mike Seymour in magic-numbers-in-boosthash-combine: http://stackoverflow.com/questions/4948780
    template <class T>
    inline void hash_combine(std::size_t& seed, T const& value)
    {
        seed ^= TupleHasher<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    // Recursive template code derived from Matthieu M.
    template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
    struct HashValueImpl
    {
        static void apply(size_t& seed, Tuple const& tuple)
        {
            HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
            hash_combine(seed, std::get<Index>(tuple));
        }
    };

    template <class Tuple>
    struct HashValueImpl<Tuple, 0>
    {
        static void apply(size_t& seed, Tuple const& tuple)
        {
            hash_combine(seed, std::get<0>(tuple));
        }
    };
}

template <typename TT>
struct TupleHasher
{
    size_t operator()(TT const& value) const
    {
        return std::hash<TT>()(value);
    }
};

template <typename... TT>
struct TupleHasher<std::tuple<TT...>>
{
    size_t operator()(std::tuple<TT...> const& value) const
    {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...>>::apply(seed, value);
        return seed;
    }
};
