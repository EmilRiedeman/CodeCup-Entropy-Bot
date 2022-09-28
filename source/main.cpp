#include "entropy/palindrome.hpp"

#include <algorithm>
#include <numeric>
#include <iostream>

using namespace entropy;

constexpr auto table = score_lookup_table<8, 7>();
std::array<uint, table.size()> sorted{};

int main(int, const char *[]) {
    std::iota(sorted.begin(), sorted.end(), 0);
    std::stable_sort(sorted.begin(), sorted.end(), [&](auto a, auto b){return table[a] > table[b];});

    for (String<8> s : sorted) {
        if (table[s]) s.print<7>(std::cerr) << ": " << (uint)table[s] << '\n';
    }

    return 0;
}