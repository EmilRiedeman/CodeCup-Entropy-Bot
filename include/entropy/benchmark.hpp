#pragma once

#include "board.hpp"
#include <chrono>
#include <iostream>

namespace entropy {

using time_point = std::chrono::high_resolution_clock::time_point;

struct Timer {
    const char *const name;
    time_point t_begin = std::chrono::high_resolution_clock::now();

    explicit Timer(const char *name): name(name) {}

    ~Timer() {
        auto t_end = std::chrono::high_resolution_clock::now();
        std::cerr << "Timer " << name << ": " << (double) std::chrono::duration_cast<std::chrono::microseconds>((t_end - t_begin)).count() / 1000. << "ms\n";
    }
};

template<std::size_t N, typename Function>
inline void benchmark(const char *name, Function f) {
    std::size_t i = 0;
    Timer timer(name);

    for (; i < N; ++i) f();
}

template<std::size_t N = 1'000'000'000>
inline void benchmark_board_copy() {
    Board board;
    board.place_chip({{2, 2}, 1});
    board.place_chip({{2, 3}, 5});
    benchmark<N>("Board Copy", [&board]() {
        volatile Board copy = board;
    });
}

}// namespace entropy
