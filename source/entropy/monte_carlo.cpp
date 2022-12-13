#include "entropy/monte_carlo.hpp"

namespace entropy::mcts {

FastRand RNG{};
OrderNodeBuffer order_node_buffer{};
ChaosNodeBuffer chaos_node_buffer{};

inline void do_smart_order_move(BoardState &board,
                                OrderMove::Compact *moves_buf) {
    moves_buf[0].make_pass();
    uint n = 1;
    int best_score = 0;
    //show_board(board.get_minimal_state());
    board.get_minimal_state().for_each_possible_order_move_with_score([&n, &moves_buf, &best_score](auto from, auto to, int score) {
        //std::cerr << from << to << ": " << score << '\n';
        if (score == best_score) moves_buf[n++] = {from, to};
        else if (score > best_score) {
            moves_buf[0] = {from, to};
            best_score = score;
            n = 1;
        }
    });

    board.move_chip(random_element(moves_buf, n, RNG)->create());

    //show_board(board.get_minimal_state());
}

inline void do_smart_chaos_move(BoardState &board,
                                std::array<uint8_t, BOARD_AREA> &chips,
                                uint8_t *moves_buf) {
    auto rand_it = random_element(chips.begin(), board.get_open_cells(), RNG);
    const Colour colour = *rand_it;
    *rand_it = chips[board.get_open_cells() - 1];

    uint n = 0;
    uint best_score = -1u;
    //show_board(board.get_minimal_state());
    board.get_minimal_state().for_each_possible_chaos_move_with_score(colour, [&n, &moves_buf, &best_score](auto p, uint score) {
        //std::cerr << p << ": " << score << '\n';
        if (score == best_score) moves_buf[n++] = p.p;
        else if (score < best_score) {
            moves_buf[0] = p.p;
            best_score = score;
            n = 1;
        }
    });

    board.place_chip({*random_element(moves_buf, n, RNG), colour});

    //show_board(board.get_minimal_state());
}

inline void smart_rollout_helper(BoardState &board,
                                 std::array<uint8_t, BOARD_AREA> &chips,
                                 OrderMove::Compact *o_moves_buf,
                                 uint8_t *c_moves_buf) {
    while (board.get_open_cells()) {
        do_smart_order_move(board, o_moves_buf);
        do_smart_chaos_move(board, chips, c_moves_buf);
    }
}

uint smart_rollout_order(const BoardState &original, const ChipPool &pool) {
    auto chips = pool.create_array();
    BoardState copy = original;

    OrderMove::Compact o_moves_buf[MAX_POSSIBLE_ORDER_MOVES];
    uint8_t c_moves_buf[BOARD_AREA];

    smart_rollout_helper(copy, chips, o_moves_buf, c_moves_buf);

    return copy.get_total_score();
}

uint smart_rollout_chaos(const BoardState &original, const ChipPool &pool) {
    if (!original.get_open_cells()) return original.get_total_score();
    auto chips = pool.create_array();
    BoardState copy = original;

    OrderMove::Compact o_moves_buf[MAX_POSSIBLE_ORDER_MOVES];
    uint8_t c_moves_buf[BOARD_AREA];

    do_smart_chaos_move(copy, chips, c_moves_buf);
    smart_rollout_helper(copy, chips, o_moves_buf, c_moves_buf);

    return copy.get_total_score();
}

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

    moves[unvisited++].make_pass();
    board.get_minimal_state().for_each_possible_order_move([this](auto from, auto to) {
        moves[unvisited++] = {from, to};
    });

    children.reserve(std::max(unvisited / 3, 2u));
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
        return node.branch_score(logN, uct_temperature);
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

OrderNode *ChaosNode::add_random_child(Colour colour) {
    uint index = colour - 1;

    auto it = random_element(unvisited_moves[index].begin(), unvisited_moves[index].size(), RNG);
    Position p = *it;

    *it = unvisited_moves[index].back();
    unvisited_moves[index].pop_back();
    if (unvisited_moves[index].empty()) unvisited_moves[index] = {};

    children[index].push_back(std::make_unique<OrderNode>(
            this,
            ChaosMove{p, colour}));
    return children[index].back().get();
}

OrderNode *ChaosNode::select_child(Colour colour, const float uct_temperature) const {
    const auto logN = std::log(float(visits[colour - 1]));
    return select_child_helper(children[colour - 1], [=](const auto &node) {
        return node.branch_score(logN, uct_temperature);
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

        if (c_node->is_terminal()) {
            c_node->rollout();
            break;
        }
        c_node->try_init();
        if (c_node->can_add_child(random_colour)) {
            c_node->add_random_child(random_colour)->rollout();
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
    root.try_init();
    root.set_as_root();

    while (root.can_add_child(c)) root.add_random_child(c)->rollout();

    for (uint i = 0; i < rollouts; ++i) {
        tree_search_helper(root.select_child(c, uct_temperature), uct_temperature);
    }
}

}// namespace entropy::mcts