#include "entropy/referee.hpp"

using namespace entropy;

int main(int argc, const char *args[]) {
    if (!argc) {
        start_official_game();
    }
    return 0;
}