#include "entropy/monte_carlo.hpp"

namespace entropy::mcts {

FastRand RNG{};

template <typename T, typename F>
inline T *select_child_helper(const std::vector<std::unique_ptr<T>> &vec, F &&evaluator) {
    auto best_score = std::forward<F>(evaluator)(*vec.front());
    auto child = vec.begin();

    for (auto it = child + 1; it != vec.end(); ++it) {
        auto s = std::forward<F>(evaluator)(**it);
        if (s > best_score) {
            best_score = s;
            child = it;
        }
    }

    return child->get();
}

OrderNode::OrderNode(
        ChaosNode *p,
        const ChaosMove &new_move) : board(p->board), pool(p->pool, new_move.colour), parent(p), last_move(new_move) {
    board.place_chip(new_move);
    init();
}

void OrderNode::init() {
    initialized = true;
    unvisited = 1;
    board.get_minimal_state().for_each_possible_order_move([this](auto a, auto b, auto c, auto d) {
        moves[unvisited++] = OrderMove::Compact({a, b, c, d});
    });

    children.reserve(std::max(unvisited / 3, 1u));
}

ChaosNode *OrderNode::add_random_child() {
    auto it = random_element(moves.begin(), unvisited, RNG);
    auto move = *it;
    *it = moves[--unvisited];

    children.push_back(std::make_unique<ChaosNode>(this, move.create()));
    return children.back().get();
}

ChaosNode *OrderNode::select_child(const float uct_temperature) const {
    const auto logN = std::log(float(total_visits));
    return select_child_helper(children, [=](const auto &node) {
        return uct_score(node.avg_score(), logN, float(node.total_visits), uct_temperature);
    });
}

OrderMove OrderNode::select_move() const {
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
        OrderNode *p, const OrderMove &new_move) : board(p->board), pool(p->pool), parent(p), last_move(new_move) {
    board.move_chip(new_move);
    init();
}

void ChaosNode::init() {
    uint m = 0;
    board.get_minimal_state().for_each_empty_space([this, &m](Position p) {
        moves[m++] = p.p;
    });
    unvisited.fill(m);
    std::shuffle(moves.begin(), moves.begin() + m, RNG);
}

OrderNode *ChaosNode::add_random_child(Colour colour) {
    children[colour - 1].push_back(std::make_unique<OrderNode>(
            this,
            ChaosMove{moves[--unvisited[colour - 1]], colour}));
    return children[colour - 1].back().get();
}

OrderNode *ChaosNode::select_child(Colour colour, const float uct_temperature) const {
    const auto logN = std::log(float(visits[colour - 1]));
    return select_child_helper(children[colour - 1], [=](const auto &node) {
        return uct_score(node.avg_score(), logN, float(node.total_visits), uct_temperature);
    });
}

ChaosMove ChaosNode::select_move(Colour colour) const {
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

inline void tree_search_helper(OrderNode *o_node, const float uct_temperature) {
    while (true) {
        o_node->try_init();
        if (o_node->can_add_child()) {
            o_node->add_random_child()->rollout();
            break;
        }
        auto c_node = o_node->select_child(uct_temperature);
        auto random_colour = c_node->random_colour();

        if (c_node->is_terminal() || c_node->can_add_child(random_colour)) {
            if (!c_node->is_terminal()) c_node->add_random_child(random_colour)->rollout();
            else c_node->rollout();
            break;
        }
        o_node = c_node->select_child(random_colour, uct_temperature);
    }
}

void tree_search_order(OrderNode &root, uint rollouts, const float uct_temperature) {
    root.try_init();
    root.set_as_root();

    while (root.can_add_child()) root.add_random_child()->rollout();

    for (uint i = 0; i < rollouts; ++i) {
        tree_search_helper(&root, uct_temperature);
    }
}

void tree_search_chaos(ChaosNode &root, Colour c, uint rollouts, const float uct_temperature) {
    if (root.is_terminal()) return;
    root.set_as_root();

    while (root.can_add_child(c)) root.add_random_child(c)->rollout();

    for (uint i = 0; i < rollouts; ++i) {
        tree_search_helper(root.select_child(c, uct_temperature), uct_temperature);
    }
}

}// namespace entropy::mcts