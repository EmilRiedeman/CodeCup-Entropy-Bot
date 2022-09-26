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
constexpr inline uint8_t count(String<C> string, int left, int right) {
    uint8_t score = 0;
    for (uint8_t a{}, b{}; left >= 0 && right < N; --left, ++right) {
        a = string.read(left);
        b = string.read(right);
        if (a == b && a && b) score += uint8_t(right - left + 1);
        else break;
    }
    return score;
}

template<uint C, uint N>
constexpr inline uint8_t score_string(String<C> string) {
    uint8_t score = 0;
    for (int i = 0; i < N; ++i) {
        if (string.read(i)) score += count<C, N>(string, i - 1, i + 1);
        score += count<C, N>(string, i, i + 1);
    }
    return score;
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