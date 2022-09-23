#pragma once

#include "data_types.hpp"
#include "util.hpp"

#include <cmath>

namespace entropy {

template<uint C>
struct String {
    static constexpr auto POWER_TABLE = generate_array<16>([](auto x, auto) { return int_pow<C>(x); });

    uint hash;

    constexpr String() = default;

    [[nodiscard]] constexpr uint read(uint index) const {
        return (hash / POWER_TABLE[index]) % C;
    }

    constexpr explicit operator uint() const {
        return hash;
    }
};

template<uint C, uint N>
constexpr bool is_palindrome(String<C> string, uint i, uint j, std::array<std::array<uint, N>, N> &dp) {
    if (i > j) return true;

    if (dp[i][j])
        return dp[i][j] - 1;

    if (string.read(i) != string.read(j)) {
        dp[i][j] = 1;
        return false;
    }

    return (dp[i][j] = is_palindrome<C, N>(string, i + 1, j - 1, dp) + 1) - 1;
}

template<uint C, uint N>
constexpr uint8_t score_string(String<C> string) {
    std::array<std::array<uint, N>, N> dp{};
    uint8_t ans = 0;

    for (uint i = 0; i < N; ++i) {
        if (!string.read(i)) continue;
        break;
        for (uint j = i + 1; j < N; ++j) {
            if (!string.read(j)) break;
            //bool b = is_palindrome<C, N>(string, i, j, dp);
            ans += 1 * uint8_t(j - i + 1);
        }
    }

    return ans;
}

template<uint C, uint N>
constexpr auto score_lookup_table() {
    std::array<std::uint8_t, LookupPow<uint>::calculate<C, N>> result{};
    for (uint i = 0; i < result.size(); ++i) {
        result[i] = score_string<C, N>({i});
    }
    return result;
    //return generate_array<>([](auto x, auto) { return score_palindrome<C, N>({x}); });
}


}