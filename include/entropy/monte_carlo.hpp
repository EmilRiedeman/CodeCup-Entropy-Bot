#pragma once

#include "board.hpp"
#include "move_maker.hpp"

#include <map>
#include <memory>
#include <utility>

namespace entropy::mcts {

extern FastRand RNG;

class MoveMaker;

class OrderNode;
class ChaosNode;

constexpr inline std::size_t PREALLOCATED_NODE_AMOUNT = 32768;

using OrderNodeBuffer = PreallocatedBuffer<OrderNode, PREALLOCATED_NODE_AMOUNT>;
using ChaosNodeBuffer = PreallocatedBuffer<ChaosNode, PREALLOCATED_NODE_AMOUNT>;

extern OrderNodeBuffer order_node_buffer;
extern ChaosNodeBuffer chaos_node_buffer;

uint smart_rollout_order(const BoardState &board, const ChipPool &pool);
uint smart_rollout_chaos(const BoardState &board, const ChipPool &pool);

constexpr inline float UCT_SCORE_MULTIPLIER = 1. / 80;

inline float uct_score(float s, float logN, float n, float temperature) {
    return s * UCT_SCORE_MULTIPLIER + temperature * std::sqrt(logN / n);
}

struct SearchEnvironment {
    float uct_temperature = 0.45;
    uint rollouts = 18'500;

    std::unordered_map<BoardHash, std::weak_ptr<OrderNode>> cached_order_nodes{};
    std::unordered_map<BoardHash, std::weak_ptr<ChaosNode>> cached_chaos_nodes{};

    std::shared_ptr<OrderNode> get_order_node(ChaosNode *parent, const ChaosMove &move);

    std::shared_ptr<ChaosNode> get_chaos_node(OrderNode *parent, const OrderMove &move);

    void tree_search_order(OrderNode &root);

    void tree_search_chaos(ChaosNode &root, Colour c);

private:
    void tree_search_helper(OrderNode *order_node, ChaosNode *chaos_root = nullptr, Colour root_colour = 0);
};

class OrderNode {
public:
    OrderNode() = delete;

    OrderNode(const BoardState &b,
              const ChipPool &pool) : board(b), pool(pool) {}

    OrderNode(ChaosNode *p,
              const ChaosMove &new_move);

    ~OrderNode();

    std::shared_ptr<ChaosNode> *get_child(const OrderMove &move) {
        auto it = std::find_if(children.begin(), children.end(),
                               [=](const auto &x) { return move == x->parents[this]; });
        if (it == children.end()) return nullptr;
        return &*it;
    }

    ChaosNode *add_random_child(SearchEnvironment &environment);

    ChaosNode *select_child(float uct_temperature) const;

    ChaosNode *select_best_node() const;

    uint rollout() const { return smart_rollout_order(board, pool); }

    [[nodiscard]] bool can_add_child() const { return unvisited; }

    void try_init() {
        if (!initialized) init();
    }

    [[nodiscard]] float average_score() const { return float(total_score) / float(total_visits); }

    [[nodiscard]] float branch_score(const float logN, float uct_temperature) const { return uct_score(-average_score(), logN, float(total_visits), uct_temperature); }

private:
    void init();

    void record_score(uint score);

    void add_parent(ChaosNode *parent, const ChaosMove &move);

    BoardState board;
    const ChipPool pool;
    std::map<ChaosNode *, ChaosMove> parents{};
    std::vector<std::shared_ptr<ChaosNode>> children;

    uint total_visits{};
    uint total_score{};

    OrderMove::Compact moves[MAX_POSSIBLE_ORDER_MOVES];
    uint unvisited{};

    bool initialized = false;

    friend SearchEnvironment;
    friend ChaosNode;
    friend MoveMaker;
};

class ChaosNode {
public:
    ChaosNode() = delete;

    ChaosNode(const BoardState &b,
              const ChipPool &pool) : board(b), pool(pool) {}

    ChaosNode(OrderNode *p,
              const OrderMove &new_move);

    ~ChaosNode() {
        for (auto &vec : children) {
            destruct_children(vec);
        }
    }

    std::shared_ptr<OrderNode> *get_child(const ChaosMove &move) {
        auto &vec = children[move.colour - 1];
        auto it = std::find_if(vec.begin(), vec.end(),
                               [=](const auto &x) { return move.pos.p == x->parents[this].pos.p; });
        if (it == vec.end()) return nullptr;
        return &*it;
    }

    OrderNode *add_random_child(Colour colour, SearchEnvironment &environment);

    [[nodiscard]] OrderNode *select_child(Colour colour, float uct_temperature) const;

    [[nodiscard]] OrderNode *select_best_node(Colour colour) const;

    [[nodiscard]] uint rollout() const { return smart_rollout_chaos(board, pool); }

    [[nodiscard]] bool can_add_child(Colour colour) const { return !unvisited_moves[colour - 1].empty(); }

    void try_init() {
        if (!initialized) init();
    }

    [[nodiscard]] float average_score() const { return float(total_score) / float(total_visits); }

    [[nodiscard]] float branch_score(const float logN, float uct_temperature) const { return uct_score(average_score(), logN, float(total_visits), uct_temperature); }

    [[nodiscard]] bool is_terminal() const { return !board.get_open_cells(); }

    [[nodiscard]] Colour random_colour() const { return pool.random_chip(RNG); }

    void clear_colours(uint keep) {
        for (uint c = 0; c < children.size(); ++c) {
            if (c == keep - 1) continue;
            unvisited_moves[c] = {};

            destruct_children(children[c]);

            total_visits -= visits[c];
            total_score -= scores[c];

            scores[c] = 0;
            visits[c] = 0;
        }
    }

private:
    void init();

    void record_score(uint score, Colour colour);

    void add_parent(OrderNode *parent, const OrderMove &move);

    void destruct_children(std::vector<std::shared_ptr<OrderNode>> &vec);

    BoardState board;
    const ChipPool pool;
    std::map<OrderNode *, OrderMove> parents{};
    std::array<std::vector<std::shared_ptr<OrderNode>>, ChipPool::N> children{};

    std::array<uint, ChipPool::N> visits{};
    uint total_visits{};
    std::array<uint, ChipPool::N> scores{};
    uint total_score{};

    std::array<std::vector<uint8_t>, ChipPool::N> unvisited_moves{};

    bool initialized = false;

    friend SearchEnvironment;
    friend OrderNode;
    friend MoveMaker;
};

class MoveMaker final : public entropy::MoveMaker {
public:
    explicit MoveMaker(SearchEnvironment environment = {}) : search_environment(std::move(environment)) {
        std::cerr << "MCTS Seed: " << RNG.seed << '\n';
    }

    ChaosMove suggest_chaos_move(Colour colour) override {
        if (!chaos_node) chaos_node = chaos_node_buffer.make_shared(board, chip_pool);
        else chaos_node->clear_colours(uint(colour));

        std::cerr << "cached visits = " << chaos_node->total_visits << '\n';
        search_environment.tree_search_chaos(*chaos_node, colour);

        auto node = chaos_node->select_best_node(colour);
        auto move = node->parents[chaos_node.get()];

        std::cerr << move.colour << move.pos;
        std::cerr << " : "
                  << "total visits = " << chaos_node->total_visits << "; node visits = " << node->total_visits << "; expected score = " << node->average_score() << '\n';

        return node->parents[chaos_node.get()];
    }

    OrderMove suggest_order_move() override {
        if (!order_node) order_node = order_node_buffer.make_shared(board, chip_pool);

        std::cerr << "cached visits = " << order_node->total_visits << '\n';
        search_environment.tree_search_order(*order_node);

        /*
        float logN = std::log(float(order_node->total_visits));

        for (const auto &c : order_node->children) {
            auto move = c->parents[order_node.get()];

            if (move.is_pass()) std::cerr << "PASS";
            else std::cerr << move.from << move.to;
            std::cerr << " : " << c->branch_score(logN, search_environment.uct_temperature) << " " << c->total_visits << " " << c->average_score() << '\n';
        }
        */

        auto node = order_node->select_best_node();
        auto move = node->parents[order_node.get()];

        if (move.is_pass()) std::cerr << "PASS";
        else std::cerr << move.from << move.to;

        std::cerr << " : "
                  << "total visits = " << order_node->total_visits << "; node visits = " << node->total_visits << "; expected score = " << node->average_score() << '\n';

        return move;
    }

    void register_chaos_move(const ChaosMove &move) override {
        board.place_chip(move);
        chip_pool = ChipPool(chip_pool, move.colour);

        if (chaos_node) {
            auto ptr = chaos_node->get_child(move);

            if (ptr) order_node = *ptr;
            else order_node = nullptr;

            chaos_node = nullptr;
        }
    }

    void register_order_move(const OrderMove &move) override {
        board.move_chip(move);

        if (order_node) {
            auto ptr = order_node->get_child(move);

            if (ptr) chaos_node = *ptr;
            else chaos_node = nullptr;

            order_node = nullptr;
        }
    }

private:
    BoardState board;
    ChipPool chip_pool;
    std::shared_ptr<OrderNode> order_node{};
    std::shared_ptr<ChaosNode> chaos_node{};

    SearchEnvironment search_environment;
};


}// namespace entropy::mcts
