#pragma once

#include "board.hpp"
#include "io_util.hpp"
#include "monte_carlo.hpp"
#include "referee.hpp"

#include <chrono>
#include <iostream>
#include <random>

namespace entropy {

using time_point = std::chrono::high_resolution_clock::time_point;

struct Timer {
    const char *const name;
    time_point t_begin = std::chrono::high_resolution_clock::now();
    time_point t_end{};

    Timer() = delete;

    explicit Timer(const char *name) : name(name) {}

    void stop() {
        t_end = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]] double millis() const {
        return (double) std::chrono::duration_cast<std::chrono::microseconds>((t_end - t_begin)).count() / 1000.;
    }

    ~Timer() {
        if (t_end < t_begin) stop();
        std::cerr << "Timer " << name << ": " << millis() << "ms\n";
    }
};

template <std::size_t N = 1'000'000'000, typename Function>
inline void benchmark(const char *name, Function &&f) {
    std::size_t i = 0;
    Timer timer(name);

    for (; i < N; ++i) std::forward<Function>(f)();
}

template <std::size_t N = 1'000'000'000, typename Function>
inline void benchmark_return_value(const char *name, Function &&f) {
    benchmark(name, [&f]() {
        [[maybe_unused]] volatile decltype(f()) v = std::forward<Function>(f)();
    });
}

template <std::size_t N = 1'000'000'000>
inline void benchmark_board_copy() {
    BoardState board;
    board.place_chip({{2, 2}, 1});
    board.place_chip({{2, 3}, 5});
    benchmark<N>("board copy", [&board]() {
        [[maybe_unused]] volatile BoardState copy = board;
    });
}

template <std::size_t N = 1'000'000'000>
inline void benchmark_rng() {
    benchmark_return_value<N>("std::rand function", &std::rand);

    FastRand gen1{};
    benchmark_return_value<N>("FastRand generator", gen1);

    std::mt19937 gen2{};
    benchmark_return_value<N>("std::mt19937 generator", gen2);
}

/*
 * R=20'000'000 N=10
 * OrderNode children vector reserve optimization
 * reserve n / 2, 1
 * 48224ms
 * 48453ms
 * 46926.3ms
 *
 * no reserve
 * 51000ms
 * 52643ms
 * 50580ms
 *
 * later init n / 2, 1
 * 47587ms
 * 46337ms
 * 46614ms
 * 46429.7ms
 *
 * later init n
 * 48309ms
 * 47531ms
 * 47831ms
 * 47838ms
 *
 * vector instead of array for unvisited_moves
 * 51009ms
 */

/* R=100'000 N=200
 * OrderNode
 * normal
 * 31418ms
 * 31574ms
 * 31471ms
 *
 * / 3
 * 30914ms
 * 31235ms
 * 31195ms
 *
 * vector instead of array for unvisited_moves
 * slow
 *
 * array instead of vector for children
 * slow
 *
 * ChaosNode
 * reserve / 3
 * 28645ms
 * 27502ms
 * 27567ms
 * 27601ms
 * 27710ms
 * 27599ms
 *
 * reserve / 2
 * 28502ms
 * 28584ms
 * slow
 *
 * reserve / 4
 * 27894ms
 * slow
 *
 * vector for moves
 * 30655ms
 * 30340ms
 * 29094ms
 * 29361ms
 *
 *
 * new
 * 28067ms
 * 27857ms
 * 27898ms
 *
 * 28898ms
 * 29835ms
 * 28870ms
 *
 * 29588ms
 * 29224ms
 * 28891ms
 */
template <std::size_t ROLLOUTS = 100'000, std::size_t N = 200>
inline void benchmark_mcts_ponder() {
    using namespace mcts;
    FastRand rand{0};
    BoardState b;
    ChipPool pool;
    RandomMoveMaker rando{1};
    for (uint i = 0; i < 15; ++i) {
        Colour c = pool.random_chip(rand);
        pool = ChipPool(pool, c);
        auto m = rando.suggest_chaos_move(c);
        b.place_chip(m);
        rando.register_chaos_move(m);
    }

    {
        mcts::RNG.seed = 0;

        Timer t("Monte Carlo tree search ponder");
        for (uint i = 0; i < N; ++i) {
            ChaosNode node(b, pool);
            tree_search_chaos(node, 1, ROLLOUTS);
        }
    }
}

inline void benchmark_simulated_game() {
    using namespace mcts;
    {
        Timer t("MCTS (chaos) vs Random (order)");
        simulate_game(mcts::MoveMaker(), RandomMoveMaker());
    }
    {
        Timer t("Random (chaos) vs MCTS (order)");
        simulate_game(RandomMoveMaker(), mcts::MoveMaker());
    }
}

}// namespace entropy
