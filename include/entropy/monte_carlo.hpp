#pragma once

#include "board.hpp"

#include <memory>

namespace entropy::mcts {

class OrderNode;
class ChaosNode;

class OrderNode {
public:
    explicit OrderNode(const Board &b) : board(b) {}

private:
    Board board;
    std::vector<std::unique_ptr<ChaosNode>> children{};
    const std::weak_ptr<ChaosNode> parent{};
};

class ChaosNode {
public:
    ChaosNode() = delete;

    ChaosNode(const Board &b, Colour c) : board(b), colour(c) { init(); }

    ChaosNode(const Board &b,
              Colour c,
              OrderNode *p,
              const Board::OrderMove &last_move) : board(b), parent(p), colour(c) {
        board.move_chip(last_move);
        init();
    };

    OrderNode *add_random_child();

private:
    void init();

    Board board;
    std::vector<std::unique_ptr<OrderNode>> children{};
    OrderNode *const parent{};
    const Colour colour;
    std::array<uint8_t, BOARD_AREA> moves{};
    uint unvisited{};
};

}// namespace entropy::mcts
