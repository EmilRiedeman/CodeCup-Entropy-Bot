#include "entropy/monte_carlo.hpp"

namespace entropy::mcts {

FastRand RNG{};

OrderNode::OrderNode(ChaosNode *p, const Board::ChaosMove &new_move) : board(p->board), pool(p->pool), parent(p) {
    board.place_chip(new_move);
    init();
}

void OrderNode::init() {
    board.for_each_possible_order_move([this](auto &&x) {
        moves[unvisited++] = Board::OrderMove::Compact(x);
    });
    std::shuffle(moves.begin(), moves.begin() + unvisited, RNG);
}

ChaosNode::ChaosNode(OrderNode *p, const Board::OrderMove &new_move, Colour c) : board(p->board), pool(p->pool, c), parent(p), colour(c) {
    board.move_chip(new_move);
    init();
}

void ChaosNode::init() {
    board.for_each_empty_space([this](Position p) {
        moves[unvisited++] = p.p;
    });
    std::shuffle(moves.begin(), moves.begin() + unvisited, RNG);
}

OrderNode *ChaosNode::add_random_child() {
    children.push_back(std::make_unique<OrderNode>(this, Board::ChaosMove{moves[--unvisited], colour}));
    return children.back().get();
}

}// namespace entropy::mcts