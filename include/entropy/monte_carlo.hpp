#pragma once

#include "board.hpp"

#include <memory>

namespace entropy::mcts {

struct OrderNode;
struct ChaosNode;

struct OrderNode {
    Board board;
    std::vector<std::shared_ptr<ChaosNode>> children{};
    const std::weak_ptr<ChaosNode> parent{};
    std::array<Board::ChaosMove, BOARD_AREA * 2> moves{};

    explicit OrderNode(const Board &b): board(b) {}
};

struct ChaosNode {
    Board board;
    std::vector<std::shared_ptr<OrderNode>> children{};
    const std::weak_ptr<OrderNode> parent{};

    explicit ChaosNode(const Board &b): board(b) {}
};

}// namespace entropy::mcts
