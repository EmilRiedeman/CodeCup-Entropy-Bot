#include "entropy/monte_carlo.hpp"

namespace entropy::mcts {

FastRand RNG{};

void ChaosNode::init() {
    board.for_each_empty_space([this](Position p) {
        moves[unvisited++] = p.p;
    });
    std::shuffle(moves.begin(), moves.begin() + unvisited, RNG);
}
OrderNode *ChaosNode::add_random_child() {
    children.push_back(std::make_unique<OrderNode>(board, this, moves[--unvisited]));
    return children.back().get();
}

}// namespace entropy::mcts