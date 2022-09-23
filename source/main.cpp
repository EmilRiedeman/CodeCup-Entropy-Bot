#include <iostream>
#include "entropy/palindrome.hpp"

using namespace entropy;

int main(int, const char *[]) {
    constexpr auto table = score_lookup_table<8, 7>();
    std::cout << (uint) table[9] << '\n';

    return 0;
}