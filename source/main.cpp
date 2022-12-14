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
            benchmark_mcts_ponder();
            //benchmark_rollout();
        }
        if (!std::strcmp(args[1], "competition")) {
            simulate_game<>(mcts::MoveMaker(7), mcts::MoveMaker(.5));

            constexpr uint N = 20;

            for (auto b : {std::pair<float, float>{.45, .25}}) {
                uint total_score = 0;
                for (uint i = 0; i < N; ++i) {
                    std::cerr << i << '\n';
                    total_score += simulate_game<>(RandomMoveMaker(), mcts::MoveMaker(b.first));
                }
                std::cerr << total_score / N << " avg score\ndone!\n\n";
            }
        }
    }
    return 0;
}