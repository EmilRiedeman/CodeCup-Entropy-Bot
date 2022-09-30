#pragma once

#include "data_types.hpp"
#include "palindrome.hpp"

#include <array>

namespace entropy {

using BoardInteger = uint;

constexpr const inline uint BOARD_SIZE = 7;
constexpr const inline uint BOARD_AREA = BOARD_SIZE * BOARD_SIZE;
constexpr const inline uint BOARD_COLOURS = 8;
constexpr const inline uint BOARD_CHIPS = BOARD_AREA / (BOARD_COLOURS - 1);

static_assert(BOARD_CHIPS * (BOARD_COLOURS - 1) == BOARD_AREA);

struct Position {
    uint p{};

    Position(uint index) : p(index) {}

    Position(uint row, uint column) : p(row * BOARD_SIZE + column) {}

    [[nodiscard]] constexpr uint index() const { return p; }

    [[nodiscard]] constexpr uint row() const { return p / BOARD_SIZE; }

    [[nodiscard]] constexpr uint column() const { return p / BOARD_SIZE; }
};

class Board {
public:
    using String = String<BOARD_COLOURS>;
    using CellArray = std::array<BoardInteger, BOARD_AREA>;
    using ScoreArray = std::array<BoardInteger, BOARD_SIZE>;
    using CellIterator = CellArray::pointer;
    using ConstCellIterator = CellArray::const_pointer;

    constexpr static auto SCORE_LOOKUP_TABLE = score_lookup_table<8, 7>();

    static constexpr BoardInteger lookup_score(String s) {
        return SCORE_LOOKUP_TABLE[s];
    }

    Board() = default;

    [[nodiscard]] ConstCellIterator get_cell_iterator(Position p) const { return &cells[p.index()]; }

    void place_chip(Position p, BoardInteger colour) {
        cells[p.index()] = colour;
        update_score_row(p.row());
        update_score_column(p.column());
    }

    void move_chip(Position from, Position to) {
        if (from.index() == to.index()) return;
        if (from.row() == to.row()) move_chip < true > (from, to.index(), to.column());
        if (from.column() == to.column()) move_chip < false > (from, to.index(), to.row());
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
                     get_sorted_string<BOARD_COLOURS, 1>(get_row_iterator(row), BOARD_SIZE));
    }

    void update_score_column(uint column) {
        update_score(vertical_score[column],
                     get_sorted_string<BOARD_COLOURS, BOARD_SIZE>(get_column_iterator(column), BOARD_SIZE));
    }

    void update_score(BoardInteger &old_score, String s) {
        BoardInteger new_score = lookup_score(s);
        total_score += new_score - old_score;
        old_score = new_score;
    }

    CellArray cells{};

    ScoreArray vertical_score{};
    ScoreArray horizontal_score{};
    uint total_score = 0;
};

}// namespace entropy
