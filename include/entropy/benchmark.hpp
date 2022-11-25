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

    Timer() = delete;

    explicit Timer(const char *name) : name(name) {}

    ~Timer() {
        auto t_end = std::chrono::high_resolution_clock::now();
        std::cerr << "Timer " << name << ": " << (double) std::chrono::duration_cast<std::chrono::microseconds>((t_end - t_begin)).count() / 1000. << "ms\n";
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
    Board board;
    board.place_chip({{2, 2}, 1});
    board.place_chip({{2, 3}, 5});
    benchmark<N>("board copy", [&board]() {
        [[maybe_unused]] volatile Board copy = board;
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

template <std::size_t ROLLOUTS = 10'000'000>
inline void benchmark_mcts_ponder() {
    using namespace mcts;
    Board b;
    ChaosNode node(b, ChipPool{});
    {
        Timer t("Monte Carlo tree search ponder");
        tree_search_chaos(node, 1, ROLLOUTS);
    }
    std::cerr << node.select_move(1).pos << '\n';
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
