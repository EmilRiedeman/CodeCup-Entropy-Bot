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

    ChaosNode *add_random_child();

private:
    void init();

    Board board;
    const ChipPool pool;
    ChaosNode *const parent{};
    std::vector<std::unique_ptr<ChaosNode>> children{};

    std::array<Board::OrderMove::Compact, BOARD_AREA * 2> moves{};// not safe size
    uint unvisited{};

    friend ChaosNode;
};

class ChaosNode {
public:
    ChaosNode() = delete;

    ChaosNode(
            const Board &b,
            const ChipPool &pool) : board(b), pool(pool) { init(); }

    ChaosNode(
            OrderNode *p,
            const Board::OrderMove &new_move);

    Colour random_colour() const { return pool.random_chip(RNG); }

    OrderNode *add_random_child(Colour colour);

    [[nodiscard]] bool can_add_child(Colour colour) const { return unvisited[colour - 1]; }

private:
    void init();

    Board board;
    const ChipPool pool;
    OrderNode *const parent{};
    std::array<std::vector<std::unique_ptr<OrderNode>>, ChipPool::N> children{};
    std::array<uint, ChipPool::N> unvisited{};

    std::array<uint8_t, BOARD_AREA> moves{};

    friend OrderNode;
};

void tree_search(ChaosNode &node, uint rollouts);

}// namespace entropy::mcts
