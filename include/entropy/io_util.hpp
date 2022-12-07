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

inline void show_board(const MinimalBoardState &b, std::ostream &out = std::cerr) {
    out << ' ';
    for (char c = 'a'; c < char('a' + BOARD_SIZE); ++c) out << ' ' << c;
    out << '\n';
    for (uint row = 0; row < BOARD_SIZE; ++row) {
        out << char('A' + row);
        for (uint column = 0; column < BOARD_SIZE; ++column) {
            auto v = b.read_chip(row, column);
            out << ' ' << char(v ? v + '0' : '_');
        }
        out << " = " << lookup_score(b.get_horizontal_string(row));
        out << '\n';
    }
    out << ' ';
    for (uint column = 0; column < BOARD_SIZE; ++column) out << " =";
    out << "\n ";
    for (uint column = 0; column < BOARD_SIZE; ++column) out << ' ' << lookup_score(b.get_vertical_string(column)) / 10;
    out << "\n ";
    for (uint column = 0; column < BOARD_SIZE; ++column) out << ' ' << lookup_score(b.get_vertical_string(column)) % 10;
    out << "   " << b.get_total_score() << "\n\n";
}

inline void show_chip_pool(const ChipPool &chip_pool, std::ostream &out = std::cerr) {
    for (uint c = 1; c <= ChipPool::N; ++c) {
        uint amount = chip_pool.chips_left(c);
        if (amount) out << c << ':' << amount << ',';
    }
    out << '\n';
}

}// namespace entropy