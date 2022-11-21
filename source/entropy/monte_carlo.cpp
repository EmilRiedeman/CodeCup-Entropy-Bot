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

inline uint simulate_score(const Board &b) {
    return b.get_total_score();
}

inline float uct_score(float avg_score, float logN, float n, float temperature) {
    return avg_score * temperature * std::sqrt(logN / n);
}

void tree_search_chaos(ChaosNode &root, Colour c, uint rollouts) {
    if (root.is_terminal()) return;

    while (root.can_add_child(c)) root.add_random_child(c);

    for (uint i = 0; i < rollouts; ++i) {
        OrderNode *o_node;
        ChaosNode *c_node;

        o_node = root.select_child(c);

        while (true) {
            if (o_node->can_add_child()) {
                o_node->add_random_child();
                break;
            }
            c_node = o_node->select_child();
            auto random_colour = c_node->random_colour();

            if (c_node->can_add_child(random_colour)) {
                c_node->add_random_child(random_colour);
                break;
            }
            o_node = c_node->select_child(random_colour);
        }
    }
}

}// namespace entropy::mcts