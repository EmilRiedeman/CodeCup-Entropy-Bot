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
            benchmark_simulated_game();
        }
        if (!std::strcmp(args[1], "competition")) {
            simulate_game<true>(mcts::MoveMaker(), mcts::MoveMaker());
            simulate_game<>(RandomMoveMaker(), mcts::MoveMaker());
            simulate_game<>(mcts::MoveMaker(), RandomMoveMaker());
        }
    }
    return 0;
}