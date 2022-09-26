#include <iostream>
#include "entropy/palindrome.hpp"

using namespace entropy;

int main(int, const char *[]) {
    constexpr auto table = score_lookup_table<8, 5>();
    std::cout << (uint) table[0b10001] << '\n';

    return 0;
}