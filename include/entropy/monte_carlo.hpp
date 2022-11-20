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

    ChaosNode *select_child() const;

    ChaosNode *add_random_child();

    [[nodiscard]] bool can_add_child() const { return unvisited; }

private:
    void init();

    Board board;
    const ChipPool pool;
    ChaosNode *const parent{};
    std::vector<std::unique_ptr<ChaosNode>> children{};

    std::array<Board::OrderMove::Compact, BOARD_AREA * 2> moves{};// not safe size
    uint unvisited{};

    uint total_visits{};
    uint total_score{};

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

    OrderNode *select_child(Colour colour) const;

    OrderNode *add_random_child(Colour colour);

    [[nodiscard]] bool can_add_child(Colour colour) const { return unvisited[colour - 1]; }

    [[nodiscard]] bool is_terminal() const { return !board.get_open_cells(); }

private:
    void init();

    Board board;
    const ChipPool pool;
    OrderNode *const parent{};
    std::array<std::vector<std::unique_ptr<OrderNode>>, ChipPool::N> children{};
    std::array<uint, ChipPool::N> unvisited{};

    std::array<uint, ChipPool::N> visits{};
    uint total_visits{};
    std::array<uint, ChipPool::N> scores{};
    uint total_score{};

    std::array<uint8_t, BOARD_AREA> moves{};

    friend OrderNode;
};

void tree_search_chaos(ChaosNode &node, Colour c, uint rollouts);

}// namespace entropy::mcts
