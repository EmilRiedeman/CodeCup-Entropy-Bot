#pragma once

#include "data_types.hpp"

#include <array>

namespace entropy {

using small_int = uint;

constexpr const inline uint BOARD_SIZE = 7;
constexpr const inline uint BOARD_COLOURS = 8;

class Board {
public:
    //small_int score_range() const;

private:
    std::array<small_int, BOARD_SIZE * BOARD_SIZE> cells{};
    std::array<small_int, BOARD_SIZE> vertical_score{};
    std::array<small_int, BOARD_SIZE> horizontal_score{};
    uint total_score = 0;
};

}// namespace entropy
