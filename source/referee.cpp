#include "entropy/referee.hpp"

namespace entropy {

void start_official_game() {
    std::string s;
    std::cin >> s;

    if (std::isdigit(s[0])) start_as_order(s[0] - '0', read_position(std::string_view(s).substr(1, 2)));
    else start_as_chaos();
}

void start_as_chaos() {
}

void start_as_order(uint colour, Position pos) {

}

}