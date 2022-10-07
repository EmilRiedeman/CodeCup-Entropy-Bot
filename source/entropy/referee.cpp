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

            chaos.register_order_move(Board::OrderMove::create(from, to));
        }
        std::cin >> c;

        auto m = chaos.suggest_move(c);
        chaos.register_chaos_move(m);

        print_position(m.pos, std::cout);
        std::cout << std::endl;
    }
}

template<typename ORDER, typename... Args>
void start_as_order(const Board::ChaosMove &first_move, Args &&...args) {
    ORDER order(std::forward<Args>(args)...);
    order.register_chaos_move(first_move);
    char str[5];
    for (uint move = 0; move < BOARD_AREA; ++move) {
        if (move) {
            std::cin >> str;
            uint colour = str[0] - '0';
            auto pos = read_position(str + 1);

            order.register_chaos_move({pos, colour});
        }
        auto m = order.suggest_move();
        order.register_order_move(m);

        print_position(m.from, std::cout);
        print_position(m.to(), std::cout);
        std::cout << std::endl;
    }
}

void start_console_game() {
    std::string s;
    std::cin >> s;

    std::random_device rd;
    if (std::isdigit(s[0])) start_as_order<RandomOrder>({read_position(std::string_view(s).substr(1, 2)), BoardInteger(s[0] - '0')}, rd());
    else start_as_chaos<RandomChaos>(rd());
}

}// namespace entropy