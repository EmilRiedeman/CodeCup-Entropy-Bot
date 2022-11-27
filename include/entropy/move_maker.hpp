#pragma once

#include "board.hpp"
#include "io_util.hpp"

#include <random>

namespace entropy {

class MoveMaker {
public:
    virtual void register_chaos_move(const ChaosMove &) {}

    virtual void register_order_move(const OrderMove &) {}

    virtual ChaosMove suggest_chaos_move(Colour colour) = 0;

    virtual OrderMove suggest_order_move() = 0;
};

class RandomMoveMaker final : public MoveMaker {
public:
    explicit RandomMoveMaker(uint seed) : gen{seed} { std::cerr << "RandomMoveMaker Seed: " << gen.seed << '\n'; }

    RandomMoveMaker() : RandomMoveMaker(std::random_device()()) {}

    ChaosMove suggest_chaos_move(Colour colour) override {
        const uint r = std::uniform_int_distribution<uint>(0, board.get_open_cells() - 1)(gen);
        auto begin = board.get_minimal_state().cells_begin();
        auto end = board.get_minimal_state().cells_end();
        uint i = 0;
        for (auto it = begin; it != end; ++it)
            if (!*it && i++ == r) return {Position::IntType(it - begin), colour};
        return {};
    }

    OrderMove suggest_order_move() override {
        std::vector<OrderMove> possible_moves{{}};

        board.get_minimal_state().for_each_possible_order_move([&possible_moves](auto from, auto to) {
            possible_moves.emplace_back(OrderMove{from, to});
        });

        return *random_element(possible_moves.begin(), possible_moves.size(), gen);
    }

    void register_chaos_move(const ChaosMove &move) override { board.place_chip(move); }

    void register_order_move(const OrderMove &move) override { board.move_chip(move); }

private:
    BoardState board;
    FastRand gen{};
};


}// namespace entropy
