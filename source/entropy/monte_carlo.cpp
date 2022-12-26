#include "entropy/monte_carlo.hpp"

namespace entropy::mcts {

FastRand RNG{};
OrderNodeBuffer order_node_buffer{};
ChaosNodeBuffer chaos_node_buffer{};

inline void do_smart_order_move(MinimalBoardState &board,
                                uint &s) {
    OrderMove::Compact moves_buf[MAX_POSSIBLE_ORDER_MOVES];

    moves_buf[0].make_pass();
    uint n = 1;
    int best_score = 0;
    board.for_each_possible_order_move_with_score([&n, &moves_buf, &best_score](auto from, auto to, int score) {
        if (score >= best_score) {
            if (score > best_score) {
                best_score = score;
                n = 0;
            }
            moves_buf[n++] = {from, to};
        }
    });

    auto it = random_element(moves_buf, n, RNG);
    if (!it->is_pass()) {
        auto m = it->create();
        board.move_chip(m.from, m.to);

        s += best_score;
    }
}

inline void do_smart_chaos_move(MinimalBoardState &board,
                                uint &s,
                                uint open_cells,
                                std::array<uint8_t, BOARD_AREA> &chips) {
    uint8_t moves_buf[BOARD_AREA];

    auto rand_it = random_element(chips.begin(), open_cells, RNG);
    const Colour colour = *rand_it;
    *rand_it = chips[open_cells - 1];

    uint n = 0;
    uint best_score = -1u;
    board.for_each_possible_chaos_move_with_score(colour, [&n, &moves_buf, &best_score](auto p, uint score) {
        if (score == best_score) moves_buf[n++] = p.p;
        else if (score < best_score) {
            moves_buf[0] = p.p;
            best_score = score;
            n = 1;
        }
    });

    Position p = *random_element(moves_buf, n, RNG);
    board.place_chip(p.row(), p.column(), colour);

    s += best_score;
}

inline void smart_rollout_helper(MinimalBoardState &board,
                                 uint &score,
                                 uint open_cells,
                                 std::array<uint8_t, BOARD_AREA> &chips) {
    while (open_cells) {
        do_smart_order_move(board, score);
        do_smart_chaos_move(board, score, open_cells, chips);
        --open_cells;
    }
}

uint smart_rollout_order(const BoardState &original, const ChipPool &pool) {
    auto chips = pool.create_array();
    auto copy = original.get_minimal_state();

    uint score = original.get_total_score();
    smart_rollout_helper(copy, score, original.get_open_cells(), chips);

    return score;
}

uint smart_rollout_chaos(const BoardState &original, const ChipPool &pool) {
    if (!original.get_open_cells()) return original.get_total_score();
    auto chips = pool.create_array();
    auto copy = original.get_minimal_state();

    uint score = original.get_total_score();
    do_smart_chaos_move(copy, score, original.get_open_cells(), chips);
    smart_rollout_helper(copy, score, original.get_open_cells() - 1, chips);

    return score;
}

template <typename T, typename F>
inline T *select_child_helper(const std::vector<std::shared_ptr<T>> &vec, F &&evaluator) {
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

OrderNode::OrderNode(ChaosNode *p,
                     const ChaosMove &new_move) : board(p->board), pool(p->pool, new_move.colour), parents{{p, new_move}} {
    board.place_chip(new_move);
}

OrderNode::~OrderNode() {
    while (!children.empty()) {
        if (!children.back().unique()) children.back()->parents.erase(this);
        children.pop_back();
    }
}

void OrderNode::init() {
    initialized = true;

    moves[unvisited++].make_pass();
    board.get_minimal_state().for_each_possible_order_move([this](auto from, auto to) {
        moves[unvisited++] = {from, to};
    });

    children.reserve(std::max(unvisited / 3, 2u));
}

ChaosNode *OrderNode::add_random_child(SearchEnvironment &environment) {
    auto it = random_element(moves, unvisited, RNG);
    auto move = *it;
    *it = moves[--unvisited];

    children.push_back(environment.get_chaos_node(this, move.create()));
    return children.back().get();
}

ChaosNode *OrderNode::select_child(float uct_temperature) const {
    const auto logN = std::log(float(total_visits));
    return select_child_helper(children, [=](const auto &node) {
        return node.branch_score(logN, uct_temperature);
    });
}

ChaosNode *OrderNode::select_best_node() const {
    return select_child_helper(children, [](const auto &node) {
        return node.average_score();
    });
}

void OrderNode::record_score(uint score, uint visits) {
    total_visits += visits;
    total_score += score;

    for (auto &[parent, m] : parents) {
        parent->scores[m.colour - 1] += score;
        parent->visits[m.colour - 1] += visits;

        parent->record_score(score, visits);
    }
}

void OrderNode::add_parent(ChaosNode *parent, const ChaosMove &move) {
    parent->visits[move.colour - 1] += total_visits;
    parent->scores[move.colour - 1] += total_score;

    parent->record_score(total_score, total_visits);
    parents[parent] = move;
}

ChaosNode::ChaosNode(OrderNode *p,
                     const OrderMove &new_move) : board(p->board), pool(p->pool), parents{{p, new_move}} {
    board.move_chip(new_move);
}

void ChaosNode::init() {
    initialized = true;

    const uint N = board.get_open_cells();

    std::vector<uint8_t> possible_moves;
    possible_moves.reserve(N);

    board.get_minimal_state().for_each_empty_space([&possible_moves](Position p) {
        possible_moves.push_back(p.p);
    });

    std::vector<uint8_t> *vec{};
    for (uint i = 0; i < ChipPool::N; ++i) {
        if (pool.chips_left(i + 1)) {
            if (vec) unvisited_moves[i] = *vec;
            else {
                unvisited_moves[i] = std::move(possible_moves);
                vec = &unvisited_moves[i];
            }
            children[i].reserve(std::max(N / 3, 2u));
        }
    }
}

OrderNode *ChaosNode::add_random_child(Colour colour, SearchEnvironment &environment) {
    uint index = colour - 1;

    auto it = random_element(unvisited_moves[index].begin(), unvisited_moves[index].size(), RNG);
    Position p = *it;

    *it = unvisited_moves[index].back();
    unvisited_moves[index].pop_back();
    if (unvisited_moves[index].empty()) unvisited_moves[index] = {};

    children[index].push_back(environment.get_order_node(this, {p, colour}));
    return children[index].back().get();
}

OrderNode *ChaosNode::select_child(Colour colour, float uct_temperature) const {
    const auto logN = std::log(float(visits[colour - 1]));
    return select_child_helper(children[colour - 1], [=](const auto &node) {
        return node.branch_score(logN, uct_temperature);
    });
}

OrderNode *ChaosNode::select_best_node(Colour colour) const {
    return select_child_helper(children[colour - 1], [](const auto &node) {
        return -node.average_score();
    });
}

void ChaosNode::record_score(uint score, uint v) {
    total_visits += v;
    total_score += score;

    for (auto &[parent, m] : parents)
        parent->record_score(score, v);
}

void ChaosNode::add_parent(OrderNode *parent, const OrderMove &move) {
    parent->record_score(total_score, total_visits);
    parents[parent] = move;
}

void ChaosNode::destruct_children(std::vector<std::shared_ptr<OrderNode>> &vec) {
    while (!vec.empty()) {
        if (!vec.back().unique()) vec.back()->parents.erase(this);
        vec.pop_back();
    }
}

std::shared_ptr<OrderNode> SearchEnvironment::get_order_node(ChaosNode *parent, const ChaosMove &move) {
    auto new_hash = parent->board.get_hash();
    new_hash.decrement();
    new_hash.change_state(move.colour - 1, move.pos.index());
    auto it = cached_order_nodes.find(new_hash);
    if (it != cached_order_nodes.end()) {
        if (!it->second.expired()) {
            auto ptr = it->second.lock();
            ptr->add_parent(parent, move);

            //std::cerr << "found cached order node with " << ptr->total_visits << " visits !\n";

            return ptr;
        }
    } else it = cached_order_nodes.emplace(new_hash, std::weak_ptr<OrderNode>()).first;
    auto new_node = order_node_buffer.make_shared(parent, move);
    it->second = new_node;
    return new_node;
}

std::shared_ptr<ChaosNode> SearchEnvironment::get_chaos_node(OrderNode *parent, const OrderMove &move) {
    auto new_hash = parent->board.get_hash();
    if (!move.is_pass()) {
        auto type = parent->board.get_minimal_state().read_chip(move.from.row(), move.from.column());
        new_hash.change_state(type, move.from.index());
        new_hash.change_state(type, move.to.index());
    }
    auto it = cached_chaos_nodes.find(new_hash);
    if (it != cached_chaos_nodes.end()) {
        if (!it->second.expired()) {
            auto ptr = it->second.lock();
            ptr->add_parent(parent, move);
            return ptr;
        }
    } else it = cached_chaos_nodes.emplace(new_hash, std::weak_ptr<ChaosNode>()).first;
    auto new_node = chaos_node_buffer.make_shared(parent, move);
    it->second = new_node;
    return new_node;
}

void SearchEnvironment::tree_search_order(OrderNode &root) {
    root.try_init();

    while (root.can_add_child()) root.add_random_child(*this)->rollout();

    for (uint i = 0; i < rollouts; ++i) {
        tree_search_helper(&root);
    }
}

void SearchEnvironment::tree_search_chaos(ChaosNode &root, Colour c) {
    if (root.is_terminal()) return;
    root.try_init();

    while (root.can_add_child(c)) root.add_random_child(c, *this)->rollout();

    for (uint i = 0; i < rollouts; ++i) {
        tree_search_helper(root.select_child(c, uct_temperature));
    }
}

inline void SearchEnvironment::tree_search_helper(OrderNode *o_node) {
    while (true) {
        o_node->try_init();
        if (o_node->can_add_child()) {
            o_node->add_random_child(*this)->rollout();
            break;
        }
        auto c_node = o_node->select_child(uct_temperature);
        auto random_colour = c_node->random_colour();

        if (c_node->is_terminal()) {
            c_node->rollout();
            break;
        }
        c_node->try_init();
        if (c_node->can_add_child(random_colour)) {
            c_node->add_random_child(random_colour, *this)->rollout();
            break;
        }
        o_node = c_node->select_child(random_colour, uct_temperature);
    }
}

}// namespace entropy::mcts