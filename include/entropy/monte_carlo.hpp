#pragma once

#include "board.hpp"
#include "move_maker.hpp"

#include <memory>

namespace entropy::mcts {

extern FastRand RNG;

class MoveMaker;

class OrderNode;
class ChaosNode;

inline uint rollout_board(const BoardState &b) {
    return b.get_total_score();
}

inline float uct_score(float s, float logN, float n, float temperature) {
    return s + temperature * std::sqrt(logN / n);
}

void tree_search_order(OrderNode &root, uint rollouts, float uct_temperature = 5);
void tree_search_chaos(ChaosNode &root, Colour c, uint rollouts, float uct_temperature = 5);

class OrderNode {
public:
    OrderNode() = delete;

    OrderNode(
            const BoardState &b,
            const ChipPool &pool) : board(b), pool(pool) { init(); }

    OrderNode(
            ChaosNode *p,
            const ChaosMove &new_move);

    void set_as_root() { parent = nullptr; }

    std::unique_ptr<ChaosNode> *get_child(const OrderMove &move) {
        auto it = std::find_if(children.begin(), children.end(),
                               [=](const auto &x) { return move == x->last_move; });
        if (it == children.end()) return nullptr;
        return &*it;
    }

    ChaosNode *add_random_child();

    ChaosNode *select_child(float uct_temperature) const;

    OrderMove select_move() const;

    void rollout() { record_score(rollout_board(board)); }

    [[nodiscard]] bool can_add_child() const { return unvisited; }

    void try_init() {
        if (!initialized) init();
    }

    [[nodiscard]] float avg_score() const { return -float(total_score) / 80 / float(total_visits); }

private:
    void init();

    void record_score(uint score);

    constexpr static std::size_t MAX_POSSIBLE_MOVES = 110;// not safe size maybe ???

    BoardState board;
    const ChipPool pool;
    ChaosNode *parent{};
    std::vector<std::unique_ptr<ChaosNode>> children;

    const ChaosMove last_move{};

    uint total_visits{};
    uint total_score{};

    std::array<OrderMove::Compact, MAX_POSSIBLE_MOVES> moves{};
    uint unvisited{};

    bool initialized = false;

    friend ChaosNode;
    friend MoveMaker;
};

class ChaosNode {
public:
    ChaosNode() = delete;

    ChaosNode(
            const BoardState &b,
            const ChipPool &pool) : board(b), pool(pool) { init(); }

    ChaosNode(
            OrderNode *p,
            const OrderMove &new_move);

    void set_as_root() { parent = nullptr; }

    std::unique_ptr<OrderNode> *get_child(const ChaosMove &move) {
        auto &vec = children[move.colour - 1];
        auto it = std::find_if(vec.begin(), vec.end(),
                               [=](const auto &x) { return move.pos.p == x->last_move.pos.p; });
        if (it == vec.end()) return nullptr;
        return &*it;
    }

    OrderNode *add_random_child(Colour colour);

    [[nodiscard]] OrderNode *select_child(Colour colour, float uct_temperature) const;

    [[nodiscard]] ChaosMove select_move(Colour colour) const;

    void rollout() { record_score(rollout_board(board)); }

    [[nodiscard]] bool can_add_child(Colour colour) const { return !unvisited_moves[colour - 1].empty(); }

    void try_init() {
        if (!initialized) init();
    }

    [[nodiscard]] float avg_score() const { return float(total_score) / 80 / float(total_visits); }

    [[nodiscard]] bool is_terminal() const { return !board.get_open_cells(); }

    [[nodiscard]] Colour random_colour() const { return pool.random_chip(RNG); }

    void clear_colours(uint keep) {
        for (uint c = 0; c < children.size(); ++c) {
            if (c == keep - 1) continue;
            unvisited_moves[c].clear();
            unvisited_moves[c].shrink_to_fit();

            children[c].clear();

            total_visits -= visits[c];
            total_score -= scores[c];

            scores[c] = 0;
            visits[c] = 0;
        }
    }

private:
    void init();

    void record_score(uint score);

    BoardState board;
    const ChipPool pool;
    OrderNode *parent{};
    std::array<std::vector<std::unique_ptr<OrderNode>>, ChipPool::N> children{};

    const OrderMove last_move{};

    std::array<uint, ChipPool::N> visits{};
    uint total_visits{};
    std::array<uint, ChipPool::N> scores{};
    uint total_score{};

    std::array<std::vector<uint8_t>, ChipPool::N> unvisited_moves{};

    bool initialized = false;

    friend OrderNode;
    friend MoveMaker;
};

class MoveMaker final : public entropy::MoveMaker {
public:
    MoveMaker() {
        std::cerr << "MCTS Seed: " << RNG.seed << '\n';
    }

    ChaosMove suggest_chaos_move(Colour colour) override {
        if (!chaos_node) chaos_node = std::make_unique<ChaosNode>(board, chip_pool);
        else chaos_node->clear_colours(uint(colour));
        //std::cerr << chaos_node->total_visits << "\n";

        tree_search_chaos(*chaos_node, colour, rollouts, uct_temperature);
        return chaos_node->select_move(colour);
    }

    OrderMove suggest_order_move() override {
        if (!order_node) order_node = std::make_unique<OrderNode>(board, chip_pool);
        //std::cerr << order_node->total_visits << "\n";

        tree_search_order(*order_node, rollouts, uct_temperature);
        return order_node->select_move();
    }

    void register_chaos_move(const ChaosMove &move) override {
        board.place_chip(move);
        //show_board(board);
        chip_pool = ChipPool(chip_pool, move.colour);

        if (chaos_node) {
            auto ptr = chaos_node->get_child(move);

            if (ptr) order_node = std::move(*ptr);
            else order_node = nullptr;

            chaos_node = nullptr;
        }
    }

    void register_order_move(const OrderMove &move) override {
        board.move_chip(move);
        //show_board(board);

        if (order_node) {
            auto ptr = order_node->get_child(move);

            if (ptr) chaos_node = std::move(*ptr);
            else chaos_node = nullptr;

            order_node = nullptr;
        }
    }

private:
    BoardState board;
    ChipPool chip_pool;
    std::unique_ptr<OrderNode> order_node{};
    std::unique_ptr<ChaosNode> chaos_node{};

    uint rollouts = 125'000;
    float uct_temperature = 7;
};


}// namespace entropy::mcts
