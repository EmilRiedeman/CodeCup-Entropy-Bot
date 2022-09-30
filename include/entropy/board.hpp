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
    uint index{};
};

class Board {
public:
    typedef String<BOARD_COLOURS> String;

    Board() = default;

    [[nodiscard]] const BoardInteger *get_iterator(Position p) const { return cells.cbegin() + p.index; }

    void place_chip(uint row, uint column, BoardInteger colour) {
    }

private:
    BoardInteger *get_iterator(Position p) { return cells.begin() + p.index; }
    BoardInteger *get_row_iterator(uint row) { return cells.begin() + row * BOARD_SIZE; }
    BoardInteger *get_column_iterator(uint column) { return cells.begin() + column; }

    std::array<BoardInteger, BOARD_AREA> cells{};

    std::array<BoardInteger, BOARD_SIZE> vertical_score{};
    std::array<BoardInteger, BOARD_SIZE> horizontal_score{};
    uint total_score = 0;
};

}// namespace entropy
