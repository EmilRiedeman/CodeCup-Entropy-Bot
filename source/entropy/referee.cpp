#include "entropy/referee.hpp"

#include "entropy/io_util.hpp"
#include "entropy/monte_carlo.hpp"

#include <memory>

namespace entropy {

template <typename CHAOS, typename... Args>
void start_as_chaos(Args &&...args) {
    CHAOS chaos(std::forward<Args>(args)...);
    uint c;
    char str[5]{};
    for (uint move = 0; move < BOARD_AREA; ++move) {
        if (move) {
            std::cin >> str;
            std::cerr << str << '\n';
            auto from = position_from_string(str);
            auto to = position_from_string(str + 2);

            chaos.register_order_move(BoardState::OrderMove::create(from, to));
        }
        std::cin >> c;
        std::cerr << c << '\n';

        auto m = chaos.suggest_chaos_move(c);
        chaos.register_chaos_move(m);

        std::cout << m.pos << std::endl;
    }
}

template <typename ORDER, typename... Args>
void start_as_order(BoardState::ChaosMove last_move, Args &&...args) {
    ORDER order(std::forward<Args>(args)...);
    order.register_chaos_move(last_move);
    char str[5]{};
    for (uint move = 0; move < BOARD_AREA; ++move) {
        if (move) {
            std::cin >> str;
            std::cerr << str << '\n';
            Colour colour = str[0] - '0';
            auto pos = position_from_string(str + 1);

            last_move = BoardState::ChaosMove{pos, colour};
            order.register_chaos_move(last_move);
        }
        auto m = order.suggest_order_move();
        order.register_order_move(m);

        if (m.is_pass()) std::cout << last_move.pos << last_move.pos;
        else std::cout << m.from << m.to();
        std::cout << std::endl;
    }
}

void start_console_game() {
    std::string s;
    std::cin >> s;
    std::cerr << s << '\n';

    if (std::isdigit(s[0])) start_as_order<mcts::MoveMaker>({position_from_string(std::string_view(s).substr(1, 2)), Colour(s[0] - '0')});
    else start_as_chaos<mcts::MoveMaker>();
}

}// namespace entropy