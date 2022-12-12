#include "entropy/benchmark.hpp"
#include "entropy/monte_carlo.hpp"
#include "entropy/referee.hpp"

#include <cstring>

int main(int argc, const char *args[]) {
    using namespace entropy;

    if (argc == 1) {
        start_console_game();
    } else if (argc >= 2) {
        if (!std::strcmp(args[1], "benchmark")) {
            //benchmark_simulated_game();
            benchmark_rollout();
        }
        if (!std::strcmp(args[1], "competition")) {
            constexpr uint N = 100;

            for (auto b : {std::pair<float, float>{.5, .75}, std::pair<float, float>{.75, .5}}) {
                uint total_score = 0;
                for (uint i = 0; i < N; ++i)
                    total_score += simulate_game<>(mcts::MoveMaker(b.first), mcts::MoveMaker(b.second));
                std::cerr << total_score / N << "\n\n\n\n\n\n";
            }
        }
    }
    return 0;
}