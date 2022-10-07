#pragma once

#include "palindrome.hpp"
#include "board.hpp"

#include <iostream>

namespace entropy {

template<uint N, uint C>
inline void print_string(String<C> s, std::ostream &out = std::cerr) {
    for (auto &x: s.template to_array<N>()) out << x;
}

inline void print_position(Position p, std::ostream &out = std::cout) { out << char('A' + p.row()) << char('a' + p.column()); }

inline Position read_position(std::string_view str) { return {uint(str[0] - 'A'), uint(str[1] - 'a')}; }

inline void print_board(const Board &b, std::ostream &out = std::cerr) {
    out << ' ';
    for (char c = 'a'; c < char('a' + BOARD_SIZE); ++c) out << ' ' << c;
    out << '\n';
    auto it = b.cells_begin();
    for (uint row = 0; row < BOARD_SIZE; ++row) {
        out << char('A' + row);
        for (uint column = 0; column < BOARD_SIZE; ++column) {
            out << ' ' << char((*it) ? (*it) + '0' : '_');
            ++it;
        }
        out << " = " << b.get_horizontal_score()[row];
        out << '\n';
    }
    out << ' ';
    for (uint column = 0; column < BOARD_SIZE; ++column) out << ' ' << b.get_vertical_score()[column] / 10;
    out << "\n ";
    for (uint column = 0; column < BOARD_SIZE; ++column) out << ' ' << b.get_vertical_score()[column] % 10;
    out << "   " << b.get_total_score() << '\n';
}

}// namespace entropy