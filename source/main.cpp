#include <iostream>
#include "entropy/palindrome.hpp"

using namespace entropy;

int main(int, const char *[]) {
    constexpr auto table = score_lookup_table<8, 7>();
    std::cerr << (uint) table[9] << '\n';

    for (uint i = 0; i < table.size(); ++i) {
        if (table[i]) {
            (String<8>{i}).print<7>(std::cerr) << ": " << (uint)table[i] << '\n';
        }
    }

    return 0;
}