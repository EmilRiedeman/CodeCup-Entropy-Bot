#pragma once

#include "board.hpp"
#include "random.hpp"

namespace entropy {

constexpr auto generate_zobrist_state_table(std::uint64_t seed = 9876543210123456789ull) {
    MersenneTwisterEngine64 gen(seed);

    std::array<std::array<std::uint64_t, BOARD_AREA>, BOARD_COLOURS> result{};

    for (auto &arr : result) {
        for (auto &value : arr) value = gen();
    }

    return result;
}

class ZobristHash {
public:
    uint64_t hash;
};

}// namespace entropy
