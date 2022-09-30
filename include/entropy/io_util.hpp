#pragma once

#include "palindrome.hpp"
#include "board.hpp"

#include <iostream>

namespace entropy {

template<uint N, uint C>
void print_string(String<C> s, std::ostream &out = std::cerr) { for (auto &x: s.template to_array<N>()) out << x; }

void print_position(Position p, std::ostream &out = std::cout) { out << 'A' + p.row() << 'b' + p.column(); }

Position read_position(std::string_view str) { return {uint(str[0] - 'A'), uint(str[1] - 'a')}; }

void print_board(const Board &b, std::ostream &out = std::cerr) {
}

}// namespace entropy