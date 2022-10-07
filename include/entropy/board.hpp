#pragma once

#include "data_types.hpp"
#include "palindrome.hpp"

#include <algorithm>
#include <array>
#include <iostream>

namespace entropy {

using BoardInteger = uint;

constexpr const inline uint BOARD_SIZE = 7;
constexpr const inline uint BOARD_AREA = BOARD_SIZE * BOARD_SIZE;
constexpr const inline uint BOARD_COLOURS = 8;
constexpr const inline uint BOARD_CHIPS = BOARD_AREA / (BOARD_COLOURS - 1);

static_assert(BOARD_CHIPS * (BOARD_COLOURS - 1) == BOARD_AREA);

struct Position {
    uint p = -1u;

    Position() = default;

    Position(const Position &) = default;

    Position(uint index): p(index) {}

    Position(uint row, uint column): p(row * BOARD_SIZE + column) {}

    [[nodiscard]] constexpr bool is_none() const { return p == -1u; }

    [[nodiscard]] constexpr uint index() const { return p; }

    [[nodiscard]] constexpr uint row() const { return p / BOARD_SIZE; }

    [[nodiscard]] constexpr uint column() const { return p % BOARD_SIZE; }
};

class Board {
public:
    using BoardString = String<BOARD_COLOURS>;
    using CellArray = std::array<BoardInteger, BOARD_AREA>;
    using ScoreArray = std::array<BoardInteger, BOARD_SIZE>;
    using CellIterator = CellArray::pointer;
    using ConstCellIterator = CellArray::const_pointer;

    constexpr static auto SCORE_LOOKUP_TABLE = score_lookup_table<8, 7>();

    static constexpr BoardInteger lookup_score(BoardString s) {
        return SCORE_LOOKUP_TABLE[s];
    }

    Board() = default;

    [[nodiscard]] ConstCellIterator cells_begin() const { return cells.cbegin(); }

    [[nodiscard]] ConstCellIterator cells_end() const { return cells.cend(); }

    [[nodiscard]] uint get_open_cells() const { return open_cells; }

    [[nodiscard]] uint get_total_score() const { return total_score; }

    [[nodiscard]] const ScoreArray &get_vertical_score() const { return vertical_score; }

    [[nodiscard]] const ScoreArray &get_horizontal_score() const { return horizontal_score; }

    struct ChaosMove {
        Position pos{};
        BoardInteger colour{};
    };

    void place_chip(const ChaosMove &move) {
        cells[move.pos.index()] = move.colour;
        update_score_row(move.pos.row());
        update_score_column(move.pos.column());
        --open_cells;
    }

    struct OrderMove {
        Position from{};
        uint t_index = -1u;
        uint change{};
        bool vertical{};

        OrderMove() = default;

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

        [[nodiscard]] bool is_pass() const { return t_index == -1u; }
    };

    void move_chip(const OrderMove &move) {
        if (move.is_pass()) return;
        if (move.vertical) move_chip<true>(move.from, move.t_index, move.change);
        else move_chip<false>(move.from, move.t_index, move.change);
    }

    template<bool VERTICAL>
    void move_chip(Position from, uint t_index, uint x) {
        const auto f_index = from.index();
        const auto f_row = from.row();
        const auto f_column = from.column();

        cells[t_index] = cells[f_index];
        cells[f_index] = 0;

        update_score_row(f_row);
        update_score_column(f_column);

        if constexpr (VERTICAL) update_score_row(x);
        else update_score_column(x);
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

    void update_score(BoardInteger &old_score, BoardString s) {
        BoardInteger new_score = lookup_score(s);
        total_score += new_score - old_score;
        old_score = new_score;
    }

    CellArray cells{};
    BoardInteger open_cells = BOARD_AREA;

    ScoreArray vertical_score{};
    ScoreArray horizontal_score{};
    uint total_score = 0;
};

template<bool LEFT_TO_RIGHT, typename Function>
inline void for_each_possible_order_move_helper(const Board &b, Function f) {
    constexpr int step = LEFT_TO_RIGHT ? 1 : -1;
    constexpr uint line_start = LEFT_TO_RIGHT ? 0 : (BOARD_SIZE - 1);

    std::array<Position, BOARD_SIZE> vertical_from{};
    auto it = b.cells_begin() + (BOARD_AREA - 1) * !LEFT_TO_RIGHT;
    uint pos_index = LEFT_TO_RIGHT ? 0 : (BOARD_AREA - 1);
    for (uint row = line_start; row < BOARD_SIZE; row += step) {
        Position horizontal_from{};
        for (uint column = line_start; column < BOARD_SIZE; column += step) {
            if (*it) {
                vertical_from[column].p = horizontal_from.p = pos_index;
            } else {
                if (!horizontal_from.is_none()) f(Board::OrderMove{horizontal_from, pos_index, column, false});
                if (!vertical_from[column].is_none()) f(Board::OrderMove{vertical_from[column], pos_index, row, true});
            }

            it += step;
            pos_index += step;
        }
    }
}

template<typename Function>
inline void for_each_possible_order_move(const Board &b, Function f) {
    for_each_possible_order_move_helper<true>(b, f);
    for_each_possible_order_move_helper<false>(b, f);
}

}// namespace entropy
