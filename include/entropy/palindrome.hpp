#pragma once

#include "data_types.hpp"
#include "util.hpp"

#include <cmath>

namespace entropy {

template<uint C>
struct String {
    static constexpr auto POWER_TABLE = generate_array<32>([](auto x, auto) { return int_pow<C>(x); });

    uint hash;

    constexpr String() = default;
    constexpr String(uint x): hash(x) {}

    [[nodiscard]] constexpr uint read(uint index) const { return (hash / POWER_TABLE[index]) % C; }

    constexpr String add_copy(uint i, uint c) const { return {hash + POWER_TABLE[i] * c}; }

    constexpr void add(uint i, uint c) { hash += POWER_TABLE[i] * c; }

    constexpr operator uint() const { return hash; }

    template<uint N>
    constexpr auto to_array() const {
        std::array<uint, N> array{};
        for (uint i = 0; i < N; ++i) array[i] = read(i);
        return array;
    }
};

template<uint C, uint N, std::ptrdiff_t STEP, typename ConstIterator>
constexpr decltype(auto) get_sorted_string(ConstIterator begin) {
    uint translate[C]{1};
    String<C> s{};
    for (uint i = 0; i < N; ++i, begin += STEP) {
        if (*begin) {
            if (!translate[*begin]) translate[*begin] = translate[0]++;
            s.add(i, translate[*begin]);
        }
    }
    return s;
}

template<uint C>
constexpr inline uint8_t count(String<C> string, uint left, uint right, const uint begin, const uint end) {
    uint8_t score = 0;
    for (; left + 1 > begin && right <= end; --left, ++right) {
        if (string.read(left) == string.read(right)) score += uint8_t(right - left + 1);
        else break;
    }
    return score;
}

template<uint C>
constexpr inline uint8_t score_string(String<C> string, const uint begin, const uint end) {
    uint8_t score = 0;
    for (uint i = begin; i < end; ++i) {
        score += count<C>(string, i - 1, i + 1, begin, end);
        score += count<C>(string, i, i + 1, begin, end);
    }
    return score;
}

template<uint C, uint STOP, uint N>
constexpr void compute_score_table(
        std::array<std::uint8_t, N> &table,
        String<C> cur_sequence = {},
        uint colours = 1,
        uint begin = 0,
        uint end = 0,
        uint prev_score = 0) {
    if (end >= STOP) return;
    compute_score_table<C, STOP, N>(table, cur_sequence, colours, end + 1, end + 1, table[cur_sequence]);

    for (uint c = 1; c < colours; ++c) {
        auto next_seq = cur_sequence.add_copy(end, c);
        table[next_seq] = score_string<C>(next_seq, begin, end) + prev_score;
        compute_score_table<C, STOP, N>(table, next_seq, colours, begin, end + 1, prev_score);
    }

    if (colours >= C) return;
    auto next_seq = cur_sequence.add_copy(end, colours);
    table[next_seq] = table[cur_sequence];
    compute_score_table<C, STOP, N>(table, next_seq, colours + 1, begin, end + 1, prev_score);
}

template<uint C, uint N>
constexpr decltype(auto) score_lookup_table() {
    std::array<std::uint8_t, LookupPow<uint>::calculate<C, N>> result{};
    compute_score_table<C, N, result.size()>(result);
    return result;
}

}// namespace entropy