#pragma once

#include "data_types.hpp"
#include "util.hpp"

namespace entropy {

template <uint C, typename = void>
struct NumberString {
private:
    static constexpr auto POWER_TABLE = generate_array<32>([](auto x, auto) { return int_pow<C>(x); });

public:
    uint hash;

    [[nodiscard]] constexpr uint read(uint index) const { return (hash / POWER_TABLE[index]) % C; }

    [[nodiscard]] constexpr uint read_first() const { return hash % C; }

    [[nodiscard]] constexpr NumberString set_empty_copy(uint i, uint c) const { return {hash + POWER_TABLE[i] * c}; }

    constexpr void set_empty(uint i, uint c) { hash += POWER_TABLE[i] * c; }

    constexpr void shift_right(uint i) { hash /= POWER_TABLE[i]; }

    constexpr void shift_right_once() { shift_right(1); }

    constexpr void shift_left(uint i) { hash *= POWER_TABLE[i]; }

    constexpr operator uint() const { return hash; }

    template <uint N>
    constexpr auto to_array() const {
        std::array<uint, N> array{};
        for (uint i = 0; i < N; ++i) array[i] = read(i);
        return array;
    }
};

template <uint C>
struct NumberString<C, std::enable_if_t<__builtin_popcount(C) == 1>> {
private:
    constexpr static uint BIT_MASK = C - 1;
    constexpr static uint BITS_PER_MASK = __builtin_popcount(BIT_MASK);

public:
    uint hash;

    [[nodiscard]] constexpr uint read(uint index) const { return (hash >> (BITS_PER_MASK * index)) & BIT_MASK; }

    [[nodiscard]] constexpr uint read_first() const { return hash & BIT_MASK; }

    constexpr NumberString set_empty_copy(uint i, uint c) const { return {hash | (c << (i * BITS_PER_MASK))}; }

    constexpr void set_empty(uint i, uint c) { hash |= c << (i * BITS_PER_MASK); }

    constexpr void shift_right(uint i) { hash >>= i * BITS_PER_MASK; }

    constexpr void shift_right_once() { hash >>= BITS_PER_MASK; }

    constexpr void shift_left(uint i) { hash <<= i * BITS_PER_MASK; }

    constexpr operator uint() const { return hash; }

    template <uint N>
    constexpr auto to_array() const {
        std::array<uint, N> array{};
        for (uint i = 0; i < N; ++i) array[i] = read(i);
        return array;
    }
};


template <uint C, uint N, std::ptrdiff_t STEP, typename ConstIterator>
constexpr NumberString<C> get_palindrome_string_equivalent(ConstIterator it) {
    uint translate[C]{};

    uint &next_number = translate[0];
    next_number = 1;

    NumberString<C> s{};
    for (uint i = 0; i < N; ++i, it += STEP) {
        if (*it) {
            if (i - 2 < N && *(it - STEP * 2) != 0 && *(it - STEP) == 0) {
                s.set_empty(i - 1, next_number++);
            }
            if (!translate[*it]) translate[*it] = next_number++;
            s.set_empty(i, translate[*it]);
        }
    }
    return s;
}

template <uint C, uint N>
constexpr NumberString<C> get_palindrome_string_equivalent(NumberString<C> original) {
    uint translate[C]{1};

    NumberString<C> s{};
    for (uint i = 0; original.hash; ++i, original.shift_right_once()) {
        if (auto v = original.read_first()) {
            if (!translate[v]) translate[v] = (*translate)++;
            s.set_empty(i, translate[v]);
        }
    }
    return s;
}

template <uint C>
constexpr inline uint8_t count(NumberString<C> string, uint left, uint right, const uint begin, const uint end) {
    uint8_t score = 0;
    for (; left + 1 > begin && right <= end; --left, ++right) {
        if (string.read(left) == string.read(right)) score += uint8_t(right - left + 1);
        else break;
    }
    return score;
}

template <uint C>
constexpr inline uint8_t score_string(NumberString<C> string, const uint begin, const uint end) {
    uint8_t score = 0;
    for (uint i = begin; i < end; ++i) {
        score += count<C>(string, i - 1, i + 1, begin, end);
        score += count<C>(string, i, i + 1, begin, end);
    }
    return score;
}

template <uint C, uint STOP, uint N>
constexpr void compute_score_table(
        std::array<std::uint8_t, N> &table,
        NumberString<C> cur_sequence = {},
        uint colours = 1,
        uint begin = 0,
        uint end = 0,
        uint prev_score = 0) {
    if (end >= STOP) return;
    compute_score_table<C, STOP, N>(table, cur_sequence, colours, end + 1, end + 1, table[cur_sequence]);

    for (uint c = 1; c < colours; ++c) {
        auto next_seq = cur_sequence.set_empty_copy(end, c);
        table[next_seq] = score_string<C>(next_seq, begin, end) + prev_score;
        compute_score_table<C, STOP, N>(table, next_seq, colours, begin, end + 1, prev_score);
    }

    if (colours >= C) return;
    auto next_seq = cur_sequence.set_empty_copy(end, colours);
    table[next_seq] = table[cur_sequence];
    compute_score_table<C, STOP, N>(table, next_seq, colours + 1, begin, end + 1, prev_score);
}

template <uint C, uint N>
constexpr decltype(auto) generate_base_score_lookup_table() {
    std::array<std::uint8_t, LookupPow<uint>::calculate<C, N>> result{};
    compute_score_table<C, N, result.size()>(result);
    return result;
}

template <uint C, uint N, uint P>
constexpr decltype(auto) generate_partial_string_equivalent_lookup_table(uint prefix) {
    std::array<uint, LookupPow<uint>::calculate<C, N - P>> result{};
    auto it = result.data();

    for (uint i = 0; i < result.size(); ++i) {
        /*
        uint translate[C]{1};

        uint o = i;
        for (uint x = 0; o; x += 3, o >>= 3) {
            if (auto v = o & 0b111) {
                if (!translate[v]) translate[v] = (*translate)++;
                it->hash |= (translate[v] << x);
            }
        }

        ++it;
         */
        NumberString<C> str{i};
        str.shift_left(P);
        str.hash += prefix;
        *(it++) = get_palindrome_string_equivalent<C, N>(str);
    }

    return result;
}

template <uint C, uint N, uint P, uint... I, typename = std::enable_if_t<LookupPow<uint>::calculate<C, P> == sizeof...(I)>>
constexpr decltype(auto) generate_complete_string_equivalent_lookup_table(std::integer_sequence<uint, I...>) {
    return std::array<std::array<uint, LookupPow<uint>::calculate<C, N - P>>, sizeof...(I)>{
            generate_partial_string_equivalent_lookup_table<C, N, P>(I)...,
    };
}

template <uint C, uint N, uint P>
constexpr decltype(auto) generate_complete_string_equivalent_lookup_table() {
    return generate_complete_string_equivalent_lookup_table<C, N, P>(std::make_integer_sequence<uint, LookupPow<uint>::calculate<C, P>>());
}

}// namespace entropy