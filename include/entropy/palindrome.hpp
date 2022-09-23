#pragma once

#include "data_types.hpp"

#include <array>

namespace entropy {

template<std::size_t... Is> struct seq{};
template<std::size_t N, std::size_t... Is>
struct gen_seq : gen_seq<N-1, N-1, Is...>{};
template<std::size_t... Is>
struct gen_seq<0, Is...> : seq<Is...>{};

template<class Generator, std::size_t... Is>
constexpr auto generate_array_helper(Generator g, seq<Is...>)
-> std::array<decltype(g(std::size_t{}, sizeof...(Is))), sizeof...(Is)>
{
    return {{g(Is, sizeof...(Is))...}};
}

template<std::size_t tcount, class Generator>
constexpr auto generate_array(Generator g)
-> decltype( generate_array_helper(g, gen_seq<tcount>{}) )
{
    return generate_array_helper(g, gen_seq<tcount>{});
}

template<uint C>
constexpr std::array<uint, C + 1> PowerTable() {
    std::array<uint, C + 1> result{};
    result[0] = 1;
    for (uint i = 1; i < C + 1; result[++i] *= C);
    return result;
}

template<uint C>
struct String {
    uint hash;

    constexpr String() = default;

    [[nodiscard]] constexpr uint read(uint index) const {
        return (hash / PowerTable<C>()[index]) % C;
    }

    constexpr explicit operator uint() const {
        return hash;
    }
};

template<uint C>
constexpr bool IsPalindrome(String<C> string, uint i, uint j, std::array<std::array<uint, C>, C> &dp) {
    if (i > j) return true;

    if (dp[i][j])
        return dp[i][j] - 1;

    if (string.read(i) != string.read(j))
        dp[i][j] = 1;

    return dp[i][j] = IsPalindrome(string, i + 1, j - 1) + 1;
}

template<uint C>
constexpr uint ScorePalindrome(String<C> string) {
    std::array<std::array<uint, C>, C> dp;
    uint ans = 0;

    for (uint i = 0; i < C; ++i) {
        for (uint j = i + 1; j < C; ++j) {
            ans += IsPalindrome(string, i, j, dp) * (j - i + 1);
        }
    }

    return ans;
}

template<uint C>
constexpr std::array<uint, PowerTable<C>()[C]> ScoreLookupTable() {
    constexpr uint size = PowerTable<C>()[C];
    std::array<uint, size> result;
    for (uint i = 0; i < size; ++i) result[i] = ScorePalindrome<C>(i);
    return result;
}


}