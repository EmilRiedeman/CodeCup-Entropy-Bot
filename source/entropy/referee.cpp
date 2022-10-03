#include "entropy/referee.hpp"

#include "entropy/io_util.hpp"
#include "entropy/move_maker.hpp"

#include <memory>

namespace entropy {

template<typename CHAOS, typename... Args>
void start_as_chaos(Args &&...args) {
    CHAOS chaos(std::forward<Args>(args)...);
    uint c;
    char str[5];
    for (uint move = 0; move < BOARD_AREA; ++move) {
        if (move) {
            std::cin >> str;
            auto from = read_position(str);
            auto to = read_position(str + 2);
            chaos.register_order_move({from, to});
        }
        std::cin >> c;
        auto m = chaos.suggest_move(c);
        chaos.register_chaos_move(m);
        print_position(m.pos, std::cout);
        std::cout << std::endl;
    }
}

void start_as_order(const Board::ChaosMove &first_move) {
    char str[5];
    for (uint move = 0; move < BOARD_AREA; ++move) {
        if (move) {
            std::cin >> str;
            uint c = str[0] - '0';
            auto pos = read_position(str + 1);
            print_position(pos, std::cerr);
            std::cerr << '\n';
        }
        print_position(first_move.pos, std::cout);
        print_position(first_move.pos, std::cout);
        std::cout << std::endl;
    }
}

void start_console_game() {
    std::string s;
    std::cin >> s;

    if (std::isdigit(s[0])) start_as_order({read_position(std::string_view(s).substr(1, 2)), BoardInteger(s[0] - '0')});
    else start_as_chaos<RandomChaos>(0);
}

}// namespace entropy