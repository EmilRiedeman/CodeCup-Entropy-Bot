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
    benchmark<N>(name, [&f]() {
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

template <std::size_t ROLLOUTS = 2'000, std::size_t N = 200>
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

inline void benchmark_rollout() {
    BoardState board{};
    ChipPool pool{};
    benchmark_return_value<1'000'000>("Rollout", [=]() {
        return mcts::smart_rollout_chaos(board, pool);
    });
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
