#pragma once

#include "board.hpp"

namespace entropy {

class ChaosMoveMaker {
public:
    virtual void register_chaos_move(const Board::ChaosMove &move) {}

    virtual void register_order_move(const Board::OrderMove &move) {}

    virtual Board::ChaosMove make_move(BoardInteger colour) = 0;
};

class OrderMoveMaker {
public:
    virtual void register_chaos_move(const Board::ChaosMove &move) {}

    virtual void register_order_move(const Board::OrderMove &move) {}

    virtual Board::OrderMove make_move() = 0;
};

class RandomChaos final : public ChaosMoveMaker {
public:
    Board::ChaosMove make_move(BoardInteger colour) override {

    }

private:
    Board board;

};

} // entropy
