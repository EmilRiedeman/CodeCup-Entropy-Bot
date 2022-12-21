#pragma once

#include "random.hpp"

namespace entropy {

template <std::size_t TYPES, std::size_t POSITIONS>
constexpr auto generate_zobrist_state_table(std::uint64_t seed = 9876543210123456789ull) {
    MersenneTwisterEngine64 gen(seed);

    std::array<std::array<std::uint64_t, POSITIONS>, TYPES> result{};

    for (auto &arr : result) {
        for (auto &value : arr) value = gen();
    }

    return result;
}

template <std::size_t TYPES, std::size_t POSITIONS>
class ZobristHash {
public:
    ZobristHash() = default;

    void change_state(uint type, uint pos_index) { hash ^= STATE_TABLE[type][pos_index]; }

    void decrement() { --open_spaces; }

    [[nodiscard]] uint get_open_spaces() const { return open_spaces; }

    bool operator==(const ZobristHash &o) const { return hash == o.hash && open_spaces == o.open_spaces; }

private:
    constexpr static inline auto STATE_TABLE = generate_zobrist_state_table<TYPES, POSITIONS>();

    std::uint64_t hash = 0;
    uint open_spaces = POSITIONS;

    friend std::hash<ZobristHash>;
};

}// namespace entropy

namespace std {

template <std::size_t TYPES, std::size_t POSITIONS>
struct hash<entropy::ZobristHash<TYPES, POSITIONS>> {
    std::size_t operator()(const entropy::ZobristHash<TYPES, POSITIONS> &x) const { return std::hash<std::uint64_t>()(x.hash); }
};

}// namespace std
