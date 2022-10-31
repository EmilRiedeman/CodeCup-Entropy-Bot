#include "entropy/benchmark.hpp"
#include "entropy/io_util.hpp"
#include "entropy/referee.hpp"

#include <cstring>

using namespace entropy;

int main(int argc, const char *args[]) {
    if (argc == 1) {
        start_console_game();
    } else if (argc >= 2) {
        if (!std::strcmp(args[1], "benchmark")) {
            benchmark_board_copy<>();
        }
    }
    return 0;
}