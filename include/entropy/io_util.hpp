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
    //out << 'X';
}

}// namespace entropy