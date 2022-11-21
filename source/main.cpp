#include "entropy/benchmark.hpp"
#include "entropy/monte_carlo.hpp"
#include "entropy/referee.hpp"

#include <cstring>

int main(int argc, const char *args[]) {
    using namespace entropy;

    std::cerr << sizeof(mcts::ChaosNode) << '\n';
    std::cerr << sizeof(mcts::OrderNode) << '\n';
    std::cerr << sizeof(Board) << '\n';

    if (argc == 1) {
        start_console_game();
    } else if (argc >= 2) {
        if (!std::strcmp(args[1], "benchmark")) {
            benchmark_mcts_ponder();
        }
        if (!std::strcmp(args[1], "competition")) {
            simulate_game(mcts::MMChaos(), RandomOrder());
        }
    }
    return 0;
}