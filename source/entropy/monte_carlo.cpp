#include "entropy/monte_carlo.hpp"

namespace entropy::mcts {

FastRand RNG{};

OrderNode::OrderNode(
        ChaosNode *p,
        const Board::ChaosMove &new_move) : board(p->board), pool(p->pool, new_move.colour), parent(p) {
    board.place_chip(new_move);
    init();
}

void OrderNode::init() {
    board.for_each_possible_order_move([this](auto &&x) {
        moves[unvisited++] = Board::OrderMove::Compact(x);
    });
}

ChaosNode *OrderNode::add_random_child() {
    auto it = moves.begin() + std::uniform_int_distribution<uint>{0, --unvisited}(RNG);
    auto end = moves.begin() + unvisited;
    std::iter_swap(it, end);

    children.push_back(std::make_unique<ChaosNode>(this, end->create()));
    return children.back().get();
}

ChaosNode::ChaosNode(
        OrderNode *p, const Board::OrderMove &new_move) : board(p->board), pool(p->pool), parent(p) {
    board.move_chip(new_move);
    init();
}

void ChaosNode::init() {
    uint m = 0;
    board.for_each_empty_space([this, &m](Position p) {
        moves[m++] = p.p;
    });
    unvisited.fill(m);
    std::shuffle(moves.begin(), moves.begin() + m, RNG);
}

OrderNode *ChaosNode::add_random_child(Colour colour) {
    children[colour - 1].push_back(std::make_unique<OrderNode>(
            this,
            Board::ChaosMove{moves[--unvisited[colour - 1]], colour}));
    return children[colour - 1].back().get();
}

void tree_search(ChaosNode &node, uint rollouts) {
    for (uint i = 0; i < rollouts; ++i) {
    }
}

}// namespace entropy::mcts