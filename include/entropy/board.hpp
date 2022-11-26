#pragma once

#include "data_types.hpp"
#include "palindrome.hpp"

#include <algorithm>
#include <array>
#include <iostream>

namespace entropy {

using Colour = uint8_t;

constexpr const inline uint BOARD_SIZE = 7;
constexpr const inline uint BOARD_AREA = BOARD_SIZE * BOARD_SIZE;
constexpr const inline uint BOARD_COLOURS = 8;
constexpr const inline uint BOARD_CHIPS = BOARD_AREA / (BOARD_COLOURS - 1);

static_assert(BOARD_CHIPS * (BOARD_COLOURS - 1) == BOARD_AREA);

struct Position {
    typedef uint IntType;
    constexpr inline static IntType NONE_VALUE = std::numeric_limits<IntType>::max();

    IntType p = NONE_VALUE;

    Position() = default;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    Position(T index) : p(index) {}

    Position(uint row, uint column) : p(row * BOARD_SIZE + column) {}

    [[nodiscard]] constexpr bool is_none() const { return p == NONE_VALUE; }

    [[nodiscard]] constexpr uint index() const { return p; }

    [[nodiscard]] constexpr uint row() const { return p / BOARD_SIZE; }

    [[nodiscard]] constexpr uint column() const { return p % BOARD_SIZE; }
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

    template <typename RandomGenerator>
    constexpr Colour random_chip(RandomGenerator &&gen) const {
        return Colour(std::upper_bound(prefix_sum.begin(), prefix_sum.end(), distribution(gen)) - prefix_sum.begin() + 1);
    }
};

class BoardState {
public:
    using String = NumberString<BOARD_COLOURS>;
    using CellArray = std::array<Colour, BOARD_AREA>;
    using ScoreIntType = uint8_t;
    using ScoreArray = std::array<ScoreIntType, BOARD_SIZE>;
    using CellIterator = CellArray::pointer;
    using ConstCellIterator = CellArray::const_pointer;

    constexpr static inline auto SCORE_LOOKUP_TABLE = score_lookup_table<BOARD_COLOURS, BOARD_SIZE>();

    static constexpr ScoreIntType lookup_score(String s) {
        return SCORE_LOOKUP_TABLE[s];
    }

    BoardState() = default;

    BoardState(const BoardState &) = default;

    [[nodiscard]] ConstCellIterator cells_begin() const { return cells.cbegin(); }

    [[nodiscard]] ConstCellIterator cells_end() const { return cells.cend(); }

    [[nodiscard]] uint get_open_cells() const { return open_cells; }

    [[nodiscard]] uint get_total_score() const { return total_score; }

    [[nodiscard]] const ScoreArray &get_vertical_score() const { return vertical_score; }

    [[nodiscard]] const ScoreArray &get_horizontal_score() const { return horizontal_score; }

    struct ChaosMove {
        Position pos{};
        Colour colour{};

        bool operator==(const ChaosMove &m) const {
            return pos.p == m.pos.p && colour == m.colour;
        }
    };

    struct OrderMove {
        Position from{};
        Position::IntType t_index = Position::NONE_VALUE;
        uint change{};
        bool vertical{};

        struct Compact {
            constexpr static inline uint16_t PASS_VALUE = std::numeric_limits<uint16_t>::max();
            uint16_t data = PASS_VALUE;

            Compact() = default;

            Compact(const OrderMove &m) : data((m.from.p << 10) |
                                               (m.t_index << 4) |
                                               (m.change << 1) |
                                               m.vertical) {}

            [[nodiscard]] OrderMove create() const {
                return (data == PASS_VALUE) ? OrderMove{}
                                            : OrderMove{
                                                      data >> 10,
                                                      Position::IntType(data & 0b1111110000) >> 4,
                                                      Position::IntType(data & 0b0000001110) >> 1,
                                                      bool(data & 1)};
            }
        };

        OrderMove() = default;

        bool operator==(const OrderMove &m) const {
            return (is_pass() && m.is_pass()) || (from.p == m.from.p && t_index == m.t_index);
        }

        inline static OrderMove create(Position from, Position to) {
            OrderMove r = {from};
            if (from.p == to.p) return r;
            r.t_index = to.index();
            if (from.row() == to.row()) {
                r.vertical = false;
                r.change = to.column();
            } else {
                r.vertical = true;
                r.change = to.row();
            }
            return r;
        }

        [[nodiscard]] Position to() const { return {t_index}; }

        [[nodiscard]] bool is_pass() const { return t_index == Position::NONE_VALUE; }
    };

    void place_chip(const ChaosMove &move) {
        cells[move.pos.index()] = move.colour;
        update_score_row(move.pos.row());
        update_score_column(move.pos.column());
        --open_cells;
    }

    void move_chip(const OrderMove &move) {
        if (move.is_pass()) return;
        if (move.vertical) move_chip<true>(move.from, move.t_index, move.change);
        else move_chip<false>(move.from, move.t_index, move.change);
    }

    template <bool VERTICAL>
    void move_chip(Position from, uint t_index, uint x) {
        const auto f_index = from.index();
        const auto f_row = from.row();
        const auto f_column = from.column();

        cells[t_index] = cells[f_index];
        cells[f_index] = 0;

        if (!VERTICAL || horizontal_score[f_row]) update_score_row(f_row);
        if (VERTICAL || vertical_score[f_column]) update_score_column(f_column);

        if constexpr (VERTICAL) update_score_row(x);
        else update_score_column(x);
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

private:
    CellIterator get_cell_iterator(Position p) { return &cells[p.index()]; }

    CellIterator get_row_iterator(uint row) { return &cells[row * BOARD_SIZE]; }

    CellIterator get_column_iterator(uint column) { return &cells[column]; }

    void update_score_row(uint row) {
        update_score(horizontal_score[row],
                     get_sorted_string<BOARD_COLOURS, BOARD_SIZE, 1>(get_row_iterator(row)));
    }

    void update_score_column(uint column) {
        update_score(vertical_score[column],
                     get_sorted_string<BOARD_COLOURS, BOARD_SIZE, BOARD_SIZE>(get_column_iterator(column)));
    }

    void update_score(ScoreIntType &old_score, String s) {
        auto new_score = lookup_score(s);
        total_score += new_score - old_score;
        old_score = new_score;
    }

    template <bool LEFT_TO_RIGHT, typename Function>
    void for_each_possible_order_move_helper(Function &&f) const {
        constexpr int step = LEFT_TO_RIGHT ? 1 : -1;
        constexpr uint line_start = LEFT_TO_RIGHT ? 0 : (BOARD_SIZE - 1);

        std::array<Position, BOARD_SIZE> vertical_from{};
        auto it = cells_begin() + (BOARD_AREA - 1) * !LEFT_TO_RIGHT;
        Position::IntType pos_index = LEFT_TO_RIGHT ? 0 : (BOARD_AREA - 1);
        for (uint row = line_start; row < BOARD_SIZE; row += step) {
            Position horizontal_from{};
            for (uint column = line_start; column < BOARD_SIZE; column += step) {
                if (*it) {
                    vertical_from[column].p = horizontal_from.p = pos_index;
                } else {
                    if (!horizontal_from.is_none()) std::forward<Function>(f)(BoardState::OrderMove{horizontal_from, pos_index, column, false});
                    if (!vertical_from[column].is_none()) std::forward<Function>(f)(BoardState::OrderMove{vertical_from[column], pos_index, row, true});
                }

                it += step;
                pos_index += step;
            }
        }
    }

    CellArray cells{};

    ScoreArray vertical_score{};
    ScoreArray horizontal_score{};

    uint open_cells = BOARD_AREA;
    uint total_score = 0;
};

}// namespace entropy
