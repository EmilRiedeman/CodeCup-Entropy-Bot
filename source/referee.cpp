#include "entropy/referee.hpp"

#include <memory>

namespace entropy {

template<typename CHAOS, typename Tuple>
void start_as_chaos(Tuple &&tuple) {
    CHAOS chaos = std::apply(std::make_unique<CHAOS>, tuple);
    uint c;
    for (uint move = 0; move < BOARD_AREA; ++move) {
        std::cin >> c;
    }
}

void start_as_order(uint colour, Position pos) {
}

void start_console_game() {
    std::string s;
    std::cin >> s;

    if (std::isdigit(s[0])) start_as_order(s[0] - '0', read_position(std::string_view(s).substr(1, 2)));
    else start_as_chaos<>();
}

}