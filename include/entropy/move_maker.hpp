#pragma once

#include "board.hpp"
#include "io_util.hpp"

#include <random>

namespace entropy {

class MoveMaker {
public:
    virtual void register_chaos_move(const BoardState::ChaosMove &) {}

    virtual void register_order_move(const BoardState::OrderMove &) {}

    virtual BoardState::ChaosMove suggest_chaos_move(Colour colour) = 0;

    virtual BoardState::OrderMove suggest_order_move() = 0;
};

class RandomMoveMaker final : public MoveMaker {
public:
    RandomMoveMaker() {
        std::cerr << "Seed: " << gen.seed << '\n';
    }

    BoardState::ChaosMove suggest_chaos_move(Colour colour) override {
        const uint r = std::uniform_int_distribution<uint>(0, board.get_open_cells() - 1)(gen);
        auto begin = board.cells_begin();
        auto end = board.cells_end();
        uint i = 0;
        for (auto it = begin; it != end; ++it)
            if (!*it && i++ == r) return {Position::IntType(it - begin), colour};
        return {};
    }

    BoardState::OrderMove suggest_order_move() override {
        std::vector<BoardState::OrderMove> possible_moves{{}};

        board.for_each_possible_order_move([&possible_moves](auto &&x) {
            possible_moves.emplace_back(x);
        });

        return possible_moves[std::uniform_int_distribution<uint>(0, possible_moves.size() - 1)(gen)];
    }

    void register_chaos_move(const BoardState::ChaosMove &move) override {
        board.place_chip(move);
        //std::cerr << board.get_total_score() << '\n';
    }

    void register_order_move(const BoardState::OrderMove &move) override {
        board.move_chip(move);
        //std::cerr << board.get_total_score() << '\n';
    }

private:
    BoardState board;
    FastRand gen{};
};


}// namespace entropy
