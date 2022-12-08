#pragma once

#include "data_types.hpp"
#include "palindrome.hpp"

#include <algorithm>
#include <array>
#include <iostream>

namespace entropy {

constexpr inline uint BOARD_SIZE = 7;
constexpr inline uint BOARD_AREA = BOARD_SIZE * BOARD_SIZE;
constexpr inline uint BOARD_COLOURS = 8;
constexpr inline uint BOARD_CHIPS = BOARD_AREA / (BOARD_COLOURS - 1);

constexpr inline std::size_t MAX_POSSIBLE_ORDER_MOVES = 110;// not safe size maybe ???

static_assert(BOARD_CHIPS * (BOARD_COLOURS - 1) == BOARD_AREA);

using Colour = uint8_t;
using BoardString = NumberString<BOARD_COLOURS>;

constexpr inline auto PARTIAL_SCORE_LOOKUP_TABLE = generate_base_score_lookup_table<BOARD_COLOURS, BOARD_SIZE>();
const auto COMPLETE_SCORE_LOOKUP_TABLE = generate_complete_score_lookup_table<BOARD_COLOURS, BOARD_SIZE, 0>(PARTIAL_SCORE_LOOKUP_TABLE);

inline uint8_t lookup_score(BoardString s) {
    return COMPLETE_SCORE_LOOKUP_TABLE[0][s.hash];
}

struct Position {
    typedef uint32_t IntType;
    constexpr inline static IntType NONE_VALUE = std::numeric_limits<IntType>::max();

    IntType p = NONE_VALUE;

    Position() = default;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    Position(T index) : p(index) {}

    Position(uint row, uint column) : p(row * BOARD_SIZE + column) {}

    [[nodiscard]] constexpr bool is_none() const { return p == NONE_VALUE; }

    [[nodiscard]] constexpr IntType index() const { return p; }

    [[nodiscard]] constexpr IntType row() const { return p / BOARD_SIZE; }

    [[nodiscard]] constexpr IntType column() const { return p % BOARD_SIZE; }
};

struct ChipPool {
    typedef uint8_t IntType;
    constexpr inline static std::size_t N = BOARD_COLOURS - 1;

    std::array<IntType, N> prefix_sum{};
    mutable std::uniform_int_distribution<uint> distribution;

    ChipPool() {
        prefix_sum.front() = BOARD_CHIPS;
        for (uint i = 1; i < BOARD_COLOURS - 1; ++i) prefix_sum[i] = prefix_sum[i - 1] + BOARD_CHIPS;
        distribution = std::uniform_int_distribution<uint>(0, prefix_sum.back() - 1);
    }

    ChipPool(std::initializer_list<IntType> l) {
        std::partial_sum(l.begin(), l.begin() + N, prefix_sum.begin());
        distribution = std::uniform_int_distribution<uint>(0, prefix_sum.back() - 1);
    }

    ChipPool(const ChipPool &o, uint c) : prefix_sum(o.prefix_sum), distribution(0, prefix_sum.back() - 2) {
        for (--c; c < prefix_sum.size(); ++c) --prefix_sum[c];
    }

    uint chips_left(Colour c) const {
        if (c - 1) return prefix_sum[c - 1] - prefix_sum[c - 2];
        return prefix_sum[0];
    }

    auto create_array() const {
        std::array<uint8_t, BOARD_AREA> r{};
        auto it = r.begin();

        it = std::fill_n(it, prefix_sum[0], 1);
        for (uint i = 1; i < N; ++i) it = std::fill_n(it, prefix_sum[i] - prefix_sum[i - 1], i + 1);
        return r;
    }

    template <typename RandomGenerator>
    Colour random_chip(RandomGenerator &&gen) const {
        return Colour(std::upper_bound(prefix_sum.begin(), prefix_sum.end(), distribution(gen)) - prefix_sum.begin() + 1);
    }
};

struct ChaosMove {
    Position pos{};
    Colour colour{};

    bool operator==(const ChaosMove &m) const {
        return pos.p == m.pos.p && colour == m.colour;
    }
};

struct OrderMove {
    Position from{};
    Position to{};

    inline static OrderMove create(Position from, Position to) {
        if (from.p == to.p) return {};
        return {from, to};
    }

    bool operator==(const OrderMove &m) const { return (is_pass() && m.is_pass()) || (from.p == m.from.p && to.p == m.to.p); }

    [[nodiscard]] bool is_pass() const { return from.p == Position::NONE_VALUE; }

    [[nodiscard]] bool is_vertical() const { return from.column() == to.column(); }

    struct Compact {
        constexpr static inline uint8_t PASS_VALUE = std::numeric_limits<uint8_t>::max();
        uint8_t from = 0;
        uint8_t to = 0;

        Compact() = default;

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        Compact(T from, T to) : from(from), to(to) {}

        Compact(Position from, Position to) : from(from.p), to(to.p) {}

        Compact(const OrderMove &m) : from(m.from.p), to(m.to.p) {
            if (m.is_pass()) from = PASS_VALUE;
        }

        void make_pass() { from = PASS_VALUE; }

        [[nodiscard]] OrderMove create() const {
            return (from == PASS_VALUE) ? OrderMove{}
                                        : OrderMove{from, to};
        }
    };
};

class MinimalBoardState {
public:
    MinimalBoardState() = default;

    MinimalBoardState(const MinimalBoardState &) = default;

    [[nodiscard]] uint read_chip(uint row, uint column) const { return horizontal[row].read(column); }

    void place_chip(uint row, uint column, Colour c) {
        horizontal[row].set_at_empty(column, c);
        vertical[column].set_at_empty(row, c);
    }

    void remove_chip(uint row, uint column) {
        horizontal[row].set_empty(column);
        vertical[column].set_empty(row);
    }

    void move_chip(Position from, Position to) {
        const auto f_row = from.row();
        const auto f_column = from.column();

        const auto t_row = to.row();
        const auto t_column = to.column();

        place_chip(t_row, t_column, read_chip(f_row, f_column));
        remove_chip(f_row, f_column);
    }

    [[nodiscard]] BoardString get_horizontal_string(uint row) const { return horizontal[row]; }

    [[nodiscard]] BoardString get_vertical_string(uint column) const { return vertical[column]; }

    [[nodiscard]] uint get_score(uint row, uint column) const { return lookup_score(horizontal[row]) + lookup_score(vertical[column]); }

    [[nodiscard]] uint get_total_score() const {
        uint r = 0;
        for (uint i = 0; i < BOARD_SIZE; ++i) r += lookup_score(horizontal[i]) + lookup_score(vertical[i]);
        return r;
    }

    template <typename Function>
    void for_each_possible_order_move(Function &&f) const {
        for_each_possible_order_move_helper<true>(std::forward<Function>(f));
        for_each_possible_order_move_helper<false>(std::forward<Function>(f));
    }

    template <typename Function>
    void for_each_possible_order_move_with_score(Function &&f) const {
        for_each_possible_order_move_with_score_helper<true>(std::forward<Function>(f));
        for_each_possible_order_move_with_score_helper<false>(std::forward<Function>(f));
    }

    template <typename Function>
    void for_each_empty_space(Function &&f) const {
        Position p{0};
        for (uint row = 0; row < BOARD_SIZE; ++row) {
            auto str = horizontal[row];
            for (uint column = 0; column < BOARD_SIZE; ++column, ++p.p) {
                if (!str.read_first()) std::forward<Function>(f)(p);
                str.shift_right_once();
            }
        }
    }

    template <typename Function>
    void for_each_empty_space_with_score(Function &&f) const {
        Position p{0};
        for (uint row = 0; row < BOARD_SIZE; ++row) {
            auto str = horizontal[row];
            for (uint column = 0; column < BOARD_SIZE; ++column, ++p.p) {
                if (!str.read_first()) std::forward<Function>(f)(p, 0);
                str.shift_right_once();
            }
        }
    }

private:
    template <bool LEFT_TO_RIGHT, typename Function>
    void for_each_possible_order_move_helper(Function &&f) const {
        constexpr int STEP = LEFT_TO_RIGHT ? 1 : -1;
        constexpr uint START = LEFT_TO_RIGHT ? 0 : (BOARD_SIZE - 1);

        std::array<Position, BOARD_SIZE> vertical_from{};
        Position pos = LEFT_TO_RIGHT ? 0 : (BOARD_AREA - 1);
        for (uint row = START; row < BOARD_SIZE; row += STEP) {
            Position horizontal_from{};
            for (uint column = START; column < BOARD_SIZE; column += STEP) {
                if (read_chip(row, column)) {
                    vertical_from[column].p = horizontal_from.p = pos.p;
                } else {
                    if (!horizontal_from.is_none()) std::forward<Function>(f)(horizontal_from, pos);
                    if (!vertical_from[column].is_none()) std::forward<Function>(f)(vertical_from[column], pos);
                }

                pos.p += STEP;
            }
        }
    }

    template <bool LEFT_TO_RIGHT, typename Function>
    void for_each_possible_order_move_with_score_helper(Function &&f) const {
        constexpr int STEP = LEFT_TO_RIGHT ? 1 : -1;
        constexpr uint START = LEFT_TO_RIGHT ? 0 : (BOARD_SIZE - 1);

        std::array<Position, BOARD_SIZE> vertical_from{};
        Position pos = LEFT_TO_RIGHT ? 0 : (BOARD_AREA - 1);
        for (uint row = START; row < BOARD_SIZE; row += STEP) {
            Position horizontal_from{};
            for (uint column = START; column < BOARD_SIZE; column += STEP) {
                if (read_chip(row, column)) {
                    vertical_from[column].p = horizontal_from.p = pos.p;
                } else {
                    if (!horizontal_from.is_none()) std::forward<Function>(f)(horizontal_from, pos, 0);
                    if (!vertical_from[column].is_none()) std::forward<Function>(f)(vertical_from[column], pos, 0);
                }

                pos.p += STEP;
            }
        }
    }

    std::array<BoardString, BOARD_SIZE> horizontal{};
    std::array<BoardString, BOARD_SIZE> vertical{};
};

class BoardState {
public:
    BoardState() = default;

    BoardState(const BoardState &) = default;

    [[nodiscard]] const MinimalBoardState &get_minimal_state() const { return minimal_state; }

    [[nodiscard]] uint get_open_cells() const { return open_cells; }

    [[nodiscard]] uint get_total_score() const { return total_score; }

    void place_chip(const ChaosMove &move) {
        const auto row = move.pos.row();
        const auto column = move.pos.column();

        const uint old_score = minimal_state.get_score(row, column);

        minimal_state.place_chip(row, column, move.colour);

        total_score += minimal_state.get_score(row, column) - old_score;

        --open_cells;
    }

    void move_chip(const OrderMove &move) {
        if (move.is_pass()) return;

        if (move.is_vertical()) move_chip<true>(move.from, move.to);
        else move_chip<false>(move.from, move.to);
    }

    template <bool VERTICAL>
    void move_chip(Position from, Position to) {
        const auto f_row = from.row();
        const auto f_column = from.column();

        const auto old_score = minimal_state.get_score(f_row, f_column) +
                               lookup_score(VERTICAL ? minimal_state.get_horizontal_string(to.row()) : minimal_state.get_vertical_string(to.column()));

        minimal_state.move_chip(from, to);

        total_score += minimal_state.get_score(f_row, f_column) +
                       lookup_score(VERTICAL ? minimal_state.get_horizontal_string(to.row()) : minimal_state.get_vertical_string(to.column())) - old_score;
    }

    template <typename Function>
    void for_each_possible_order_move_score_differance(Function &&f) const {
        constexpr int UNINITIALIZED_VALUE = std::numeric_limits<int>::max();
        auto scores = create_array<BOARD_AREA>(UNINITIALIZED_VALUE);

        minimal_state.for_each_possible_order_move([&scores](auto from, auto to) {
            if (scores[from.p] == UNINITIALIZED_VALUE) scores[from.p] = 0;
        });
    }

private:
    MinimalBoardState minimal_state;

    uint open_cells = BOARD_AREA;
    uint total_score = 0;
};

}// namespace entropy
