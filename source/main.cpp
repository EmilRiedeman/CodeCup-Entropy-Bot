#include "entropy/benchmark.hpp"
#include "entropy/monte_carlo.hpp"
#include "entropy/referee.hpp"

#include <cstring>

int main(int argc, const char *args[]) {
    using namespace entropy;
    constexpr bool x = std::is_integral_v<unsigned char>;
    if (x) {
    }

    if (argc == 1) {
        start_console_game();
    } else if (argc >= 2) {
        if (!std::strcmp(args[1], "benchmark")) {
            benchmark_mcts_ponder();
        }
        if (!std::strcmp(args[1], "competition")) {
            uint total_score = 0;
            uint N = 100;
            for (uint i = 0; i < N; ++i)
                total_score += simulate_game<>(RandomMoveMaker(), mcts::MoveMaker());
            std::cerr << total_score / N << '\n';
        }
    }
    return 0;
}