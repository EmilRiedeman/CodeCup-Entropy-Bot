#include "entropy/monte_carlo.hpp"

namespace entropy::mcts {

FastRand RNG{};

OrderNode::OrderNode(
        ChaosNode *p,
        const Board::ChaosMove &new_move) : board(p->board), pool(p->pool, new_move.colour), parent(p), last_move(new_move) {
    board.place_chip(new_move);
    init();
}

void OrderNode::init() {
    unvisited = 1;
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

ChaosNode *OrderNode::select_child() const {
    const auto logN = std::log(float(total_visits));
    return select_child_helper(children, [&logN](const auto &node) {
        return uct_score(node.avg_score(), logN, float(node.total_visits), 1.5);
    });
}

Board::OrderMove OrderNode::select_move() const {
    return select_child_helper(children, [](const auto &node) {
               return node.avg_score();
           })
            ->last_move;
}

void OrderNode::record_score(uint score) {
    ++total_visits;
    total_score += score;

    if (parent) {
        parent->scores[last_move.colour - 1] += score;
        ++parent->visits[last_move.colour - 1];

        parent->record_score(score);
    }
}

ChaosNode::ChaosNode(
        OrderNode *p, const Board::OrderMove &new_move) : board(p->board), pool(p->pool), parent(p), last_move(new_move) {
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

OrderNode *ChaosNode::select_child(Colour colour) const {
    const auto logN = std::log(float(visits[colour - 1]));
    return select_child_helper(children[colour - 1], [&logN](const auto &node) {
        return uct_score(node.avg_score(), logN, float(node.total_visits), 1.5);
    });
}

Board::ChaosMove ChaosNode::select_move(Colour colour) const {
    return select_child_helper(children[colour - 1], [](const auto &node) {
               return node.avg_score();
           })
            ->last_move;
}

void ChaosNode::record_score(uint score) {
    ++total_visits;
    total_score += score;

    if (parent) parent->record_score(score);
}

inline void tree_search_helper(OrderNode *o_node) {
    while (true) {
        if (o_node->can_add_child()) {
            o_node->add_random_child()->rollout();
            break;
        }
        auto c_node = o_node->select_child();
        auto random_colour = c_node->random_colour();

        if (c_node->is_terminal() || c_node->can_add_child(random_colour)) {
            if (!c_node->is_terminal()) c_node->add_random_child(random_colour)->rollout();
            else c_node->rollout();
            break;
        }
        o_node = c_node->select_child(random_colour);
    }
}

void tree_search_order(OrderNode &root, uint rollouts) {
    root.set_as_root();

    while (root.can_add_child()) root.add_random_child()->rollout();

    for (uint i = 0; i < rollouts; ++i) {
        tree_search_helper(&root);
    }
}

void tree_search_chaos(ChaosNode &root, Colour c, uint rollouts) {
    if (root.is_terminal()) return;
    root.set_as_root();

    while (root.can_add_child(c)) root.add_random_child(c)->rollout();

    for (uint i = 0; i < rollouts; ++i) {
        tree_search_helper(root.select_child(c));
    }
}

}// namespace entropy::mcts