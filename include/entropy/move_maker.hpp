#pragma once

#include "board.hpp"
#include "io_util.hpp"

#include <random>

namespace entropy {

class ChaosMoveMaker {
public:
    virtual void register_chaos_move(const Board::ChaosMove &) {}

    virtual void register_order_move(const Board::OrderMove &) {}

    virtual Board::ChaosMove suggest_move(BoardInteger colour) = 0;
};

class OrderMoveMaker {
public:
    virtual void register_chaos_move(const Board::ChaosMove &) {}

    virtual void register_order_move(const Board::OrderMove &) {}

    virtual Board::OrderMove suggest_move() = 0;
};

class RandomChaos final : public ChaosMoveMaker {
public:
    explicit RandomChaos(uint seed): gen(seed) {
        std::cerr << "Seed: " << seed << '\n';
    }

    Board::ChaosMove suggest_move(BoardInteger colour) override {
        const uint r = std::uniform_int_distribution<uint>(0, board.get_open_cells() - 1)(gen);
        auto begin = board.cells_begin();
        auto end = board.cells_end();
        uint i = 0;
        for (auto it = begin; it != end; ++it)
            if (!*it && i++ == r) return {it - begin, colour};
        return {};
    }

    void register_chaos_move(const Board::ChaosMove &move) override {
        board.place_chip(move);
        print_board(board);
    }

    void register_order_move(const Board::OrderMove &move) override {
        board.move_chip(move);
        print_board(board);
    }

private:
    Board board;
    std::mt19937 gen;
};

class RandomOrder final : public OrderMoveMaker {
public:
    explicit RandomOrder(uint seed): gen(seed) {
        std::cerr << "Seed: " << seed << '\n';
    }

    Board::OrderMove suggest_move() override {
        std::vector<Board::OrderMove> possible_moves{{}};

        for_each_possible_order_move(board, [&possible_moves](auto &&x) {
            possible_moves.emplace_back(x);
        });

        return possible_moves[std::uniform_int_distribution<uint>(0, possible_moves.size() - 1)(gen)];
    }

    void register_chaos_move(const Board::ChaosMove &move) override {
        board.place_chip(move);
        print_board(board);
    }

    void register_order_move(const Board::OrderMove &move) override {
        board.move_chip(move);
        print_board(board);
    }

private:
    Board board;
    std::mt19937 gen;
};

}// namespace entropy