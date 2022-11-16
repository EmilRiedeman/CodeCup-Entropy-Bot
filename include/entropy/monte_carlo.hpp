#pragma once

#include "board.hpp"

#include <memory>

namespace entropy::mcts {

extern FastRand RNG;

class OrderNode;
class ChaosNode;

class OrderNode {
public:
    OrderNode() = delete;

    OrderNode(
            const Board &b,
            const ChipPool &pool) : board(b), pool(pool) { init(); }

    OrderNode(
            ChaosNode *p,
            const Board::ChaosMove &new_move);

private:
    void init();

    Board board;
    const ChipPool pool;
    ChaosNode *const parent{};
    std::vector<std::unique_ptr<ChaosNode>> children{};
    uint unvisited{};

    std::array<Board::OrderMove::Compact, BOARD_AREA * 2> moves{};// not safe size

    friend ChaosNode;
};

class ChaosNode {
public:
    ChaosNode() = delete;

    ChaosNode(
            const Board &b,
            const ChipPool &pool,
            Colour c) : board(b), pool(pool), colour(c) { init(); }

    ChaosNode(
            OrderNode *p,
            const Board::OrderMove &new_move,
            Colour c);

    OrderNode *add_random_child();

private:
    void init();

    Board board;
    const ChipPool pool;
    OrderNode *const parent{};
    std::vector<std::unique_ptr<OrderNode>> children{};
    uint unvisited{};

    const Colour colour;
    std::array<uint8_t, BOARD_AREA> moves{};

    friend OrderNode;
};

}// namespace entropy::mcts
