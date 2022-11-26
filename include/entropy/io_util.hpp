#pragma once

#include "board.hpp"
#include "palindrome.hpp"

#include <iostream>

namespace entropy {

inline std::ostream &operator<<(std::ostream &out, uint8_t x) { return out << uint(x); }

constexpr char *position_to_string(Position p, char *dest) {
    dest[0] = char('A' + p.row());
    dest[1] = char('a' + p.column());
    return dest;
}

inline Position position_from_string(std::string_view str) { return {uint(str[0] - 'A'), uint(str[1] - 'a')}; }

inline std::ostream &operator<<(std::ostream &out, Position p) {
    char str[3]{};
    return out << position_to_string(p, str);
}

inline std::istream &operator>>(std::istream &in, Position &p) {
    char str[3]{};
    in >> str;
    p = position_from_string(str);
    return in;
}

inline void show_board(const BoardState &b, std::ostream &out = std::cerr) {
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
    for (uint column = 0; column < BOARD_SIZE; ++column) out << " =";
    out << "\n ";
    for (uint column = 0; column < BOARD_SIZE; ++column) out << ' ' << b.get_vertical_score()[column] / 10;
    out << "\n ";
    for (uint column = 0; column < BOARD_SIZE; ++column) out << ' ' << b.get_vertical_score()[column] % 10;
    out << "   " << b.get_total_score() << '\n';
}

}// namespace entropy