#include "entropy/io_util.hpp"
#include "entropy/referee.hpp"

#include <iostream>

using namespace entropy;

int main(int argc, const char *args[]) {
    if (argc == 1) {
        start_console_game();
    }
    return 0;
}