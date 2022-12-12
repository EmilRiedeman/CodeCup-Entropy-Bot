#pragma once

#include "board.hpp"
#include "io_util.hpp"

namespace entropy {

template <bool PRINT = false, typename CHAOS, typename ORDER>
inline uint simulate_game(CHAOS &&chaos, ORDER &&order) {
    BoardState b;
    FastRand rand{};
    std::cerr << "Simulation game seed: " << rand.seed << '\n';
    ChipPool pool;

    for (uint move = 0; move < BOARD_AREA; ++move) {
        if constexpr (PRINT) show_chip_pool(pool);

        if (move) {
            auto order_move = std::forward<ORDER>(order).suggest_order_move();

            b.move_chip(order_move);
            std::forward<CHAOS>(chaos).register_order_move(order_move);
            std::forward<ORDER>(order).register_order_move(order_move);

            if constexpr (PRINT) show_board(b.get_minimal_state());
        }

        auto c = pool.random_chip(rand);
        pool = ChipPool(pool, c);

        auto chaos_move = std::forward<CHAOS>(chaos).suggest_chaos_move(c);

        b.place_chip(chaos_move);
        std::forward<CHAOS>(chaos).register_chaos_move(chaos_move);
        std::forward<ORDER>(order).register_chaos_move(chaos_move);

        if constexpr (PRINT) show_board(b.get_minimal_state());
    }

    //show_board(b.get_minimal_state());
    std::cerr << b.get_total_score() << '\n';

    return b.get_total_score();
}

void start_console_game();

}// namespace entropy