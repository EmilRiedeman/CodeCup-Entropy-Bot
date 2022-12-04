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
constexpr inline auto PALINDROME_STRING_EQUIVALENT_LOOKUP_TABLE = generate_complete_string_equivalent_lookup_table<BOARD_COLOURS, BOARD_SIZE, 2>();

constexpr uint8_t lookup_score(BoardString s) {
    uint i2 = s.read_first(BOARD_COLOURS - 2);
    s.shift_right(BOARD_COLOURS - 2);
    return PARTIAL_SCORE_LOOKUP_TABLE[PALINDROME_STRING_EQUIVALENT_LOOKUP_TABLE[s.hash][i2]];
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
    using CellArray = std::array<Colour, BOARD_AREA>;
    using CellIterator = CellArray::pointer;
    using ConstCellIterator = CellArray::const_pointer;

    MinimalBoardState() = default;

    MinimalBoardState(const MinimalBoardState &) = default;

    void place_chip(Position p, Colour c) { cells[p.index()] = c; }

    void remove_chip(Position p) { cells[p.index()] = 0; }

    void move_chip(Position from, Position to) {
        cells[to.index()] = cells[from.index()];
        cells[from.index()] = 0;
    }

    [[nodiscard]] uint8_t get_horizontal_score(uint row) const { return lookup_score(get_palindrome_string_equivalent<BOARD_COLOURS, BOARD_SIZE, 1>(&cells[row * BOARD_SIZE])); }

    [[nodiscard]] uint8_t get_vertical_score(uint column) const { return lookup_score(get_palindrome_string_equivalent<BOARD_COLOURS, BOARD_SIZE, BOARD_SIZE>(&cells[column])); }

    [[nodiscard]] uint get_total_score() const {
        uint r = 0;
        for (uint i = 0; i < BOARD_SIZE; ++i) r += get_horizontal_score(i) + get_vertical_score(i);
        return r;
    }

    template <typename Function>
    void for_each_possible_order_move(Function &&f) const {
        for_each_possible_order_move_helper<true>(std::forward<Function>(f));
        for_each_possible_order_move_helper<false>(std::forward<Function>(f));
    }

    template <typename Function>
    void for_each_empty_space(Function &&f) const {
        auto begin = cells_begin();
        auto end = cells_end();
        Position::IntType p{};
        for (auto it = begin; it != end; ++it, ++p)
            if (!*it) std::forward<Function>(f)(Position{p});
    }

    [[nodiscard]] ConstCellIterator cells_begin() const { return cells.cbegin(); }

    [[nodiscard]] ConstCellIterator cells_end() const { return cells.cend(); }

private:
    template <bool LEFT_TO_RIGHT, typename Function>
    void for_each_possible_order_move_helper(Function &&f) const {
        constexpr int step = LEFT_TO_RIGHT ? 1 : -1;
        constexpr uint line_start = LEFT_TO_RIGHT ? 0 : (BOARD_SIZE - 1);

        std::array<Position, BOARD_SIZE> vertical_from{};
        auto it = cells_begin() + (BOARD_AREA - 1) * !LEFT_TO_RIGHT;
        Position pos = LEFT_TO_RIGHT ? 0 : (BOARD_AREA - 1);
        for (uint row = line_start; row < BOARD_SIZE; row += step) {
            Position horizontal_from{};
            for (uint column = line_start; column < BOARD_SIZE; column += step) {
                if (*it) {
                    vertical_from[column].p = horizontal_from.p = pos.p;
                } else {
                    if (!horizontal_from.is_none()) std::forward<Function>(f)(horizontal_from, pos);
                    if (!vertical_from[column].is_none()) std::forward<Function>(f)(vertical_from[column], pos);
                }

                it += step;
                pos.p += step;
            }
        }
    }

    CellArray cells{};
};

class BoardState {
public:
    using ScoreArray = std::array<uint8_t, BOARD_SIZE>;

    BoardState() = default;

    BoardState(const BoardState &) = default;

    [[nodiscard]] const MinimalBoardState &get_minimal_state() const { return minimal_state; }

    [[nodiscard]] uint get_open_cells() const { return open_cells; }

    [[nodiscard]] uint get_total_score() const { return total_score; }

    [[nodiscard]] const ScoreArray &get_vertical_score() const { return vertical_score; }

    [[nodiscard]] const ScoreArray &get_horizontal_score() const { return horizontal_score; }

    void place_chip(const ChaosMove &move) {
        minimal_state.place_chip(move.pos, move.colour);

        update_horizontal_score(move.pos.row());
        update_vertical_score(move.pos.column());
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

        minimal_state.move_chip(from, to);

        if (!VERTICAL || horizontal_score[f_row]) update_horizontal_score(f_row);
        if (VERTICAL || vertical_score[f_column]) update_vertical_score(f_column);

        if constexpr (VERTICAL) update_horizontal_score(to.row());
        else update_vertical_score(to.column());
    }

    int remove_chip_score_differance(Position pos) const {
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
    void update_horizontal_score(uint row) { update_score(horizontal_score[row], minimal_state.get_horizontal_score(row)); }

    void update_vertical_score(uint column) { update_score(vertical_score[column], minimal_state.get_vertical_score(column)); }

    void update_score(ScoreArray::value_type &old_score, ScoreArray::value_type new_score) {
        total_score += new_score - old_score;
        old_score = new_score;
    }

    MinimalBoardState minimal_state;

    ScoreArray vertical_score{};
    ScoreArray horizontal_score{};

    uint open_cells = BOARD_AREA;
    uint total_score = 0;
};

}// namespace entropy
