#pragma once

#include "board.hpp"
#include "move_maker.hpp"

#include <memory>

namespace entropy::mcts {

extern FastRand RNG;

class MMChaos;

class OrderNode;
class ChaosNode;

inline uint rollout_board(const Board &b) {
    return b.get_total_score();
}

inline float uct_score(float s, float logN, float n, float temperature = 1.5) {
    return s * temperature * std::sqrt(logN / n);
}

void tree_search_order(OrderNode &root, uint rollouts);
void tree_search_chaos(ChaosNode &node, Colour c, uint rollouts);

template <typename T, typename F>
inline T *select_child_helper(const std::vector<std::unique_ptr<T>> &vec, F &&evaluator) {
    auto best_score = std::forward<F>(evaluator)(*vec.front().get());
    auto child = vec.front().get();

    for (auto it = vec.begin() + 1; it != vec.end(); ++it) {
        auto s = std::forward<F>(evaluator)(**it);
        if (s > best_score) {
            best_score = s;
            child = it->get();
        }
    }

    return child;
}

class OrderNode {
public:
    OrderNode() = delete;

    OrderNode(
            const Board &b,
            const ChipPool &pool) : board(b), pool(pool) { init(); }

    OrderNode(
            ChaosNode *p,
            const Board::ChaosMove &new_move);

    void set_as_root() { parent = nullptr; }

    std::unique_ptr<ChaosNode> *get_child(const Board::OrderMove &move) {
        auto it = std::find_if(children.begin(), children.end(),
                               [=](const auto &x) { return move == x->last_move; });
        if (it == children.end()) return nullptr;
        return &*it;
    }

    ChaosNode *add_random_child();

    ChaosNode *select_child() const;

    Board::OrderMove select_move() const;

    void rollout() { record_score(rollout_board(board)); }

    [[nodiscard]] bool can_add_child() const { return unvisited; }

    [[nodiscard]] float avg_score() const { return 5 - float(total_score) / 80 / float(total_visits); }

private:
    void init();

    void record_score(uint score);

    Board board;
    const ChipPool pool;
    ChaosNode *parent{};
    std::vector<std::unique_ptr<ChaosNode>> children{};

    const Board::ChaosMove last_move{};

    uint total_visits{};
    uint total_score{};

    std::array<Board::OrderMove::Compact, 108> moves{};// not safe size maybe ???
    uint unvisited{};

    friend ChaosNode;
    friend MMChaos;
};

class ChaosNode {
public:
    ChaosNode() = delete;

    ChaosNode(
            const Board &b,
            const ChipPool &pool) : board(b), pool(pool) { init(); }

    ChaosNode(
            OrderNode *p,
            const Board::OrderMove &new_move);

    void set_as_root() { parent = nullptr; }

    std::unique_ptr<OrderNode> *get_child(const Board::ChaosMove &move) {
        auto &vec = children[move.colour - 1];
        auto it = std::find_if(vec.begin(), vec.end(),
                               [=](const auto &x) { return move.pos.p == x->last_move.pos.p; });
        if (it == vec.end()) return nullptr;
        return &*it;
    }

    OrderNode *add_random_child(Colour colour);

    OrderNode *select_child(Colour colour) const;

    Board::ChaosMove select_move(Colour colour) const;

    void rollout() { record_score(rollout_board(board)); }

    [[nodiscard]] bool can_add_child(Colour colour) const { return unvisited[colour - 1]; }

    [[nodiscard]] float avg_score() const { return float(total_score) / 80 / float(total_visits); }

    [[nodiscard]] bool is_terminal() const { return !board.get_open_cells(); }

    Colour random_colour() const { return pool.random_chip(RNG); }

    void clear_colours(uint keep) {
        for (uint c = 0; c < children.size(); ++c) {
            if (c == keep - 1) continue;
            unvisited[c] += children[c].size();

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

    Board board;
    const ChipPool pool;
    OrderNode *parent{};
    std::array<std::vector<std::unique_ptr<OrderNode>>, ChipPool::N> children{};

    const Board::OrderMove last_move{};

    std::array<uint, ChipPool::N> visits{};
    uint total_visits{};
    std::array<uint, ChipPool::N> scores{};
    uint total_score{};

    std::array<uint8_t, BOARD_AREA> moves{};
    std::array<uint8_t, ChipPool::N> unvisited{};

    friend OrderNode;
    friend MMChaos;
};

class MMChaos final : public ChaosMoveMaker {
public:
    MMChaos() {
        std::cerr << "MCTS Seed: " << RNG.seed << '\n';
    }

    Board::ChaosMove suggest_move(Colour colour) override {
        if (!chaos_node) chaos_node = std::make_unique<ChaosNode>(board, chip_pool);
        else {
            chaos_node->clear_colours(uint(colour));
        }
        std::cerr << "Already visited " << chaos_node->total_visits << " times in current node\n";

        tree_search_chaos(*chaos_node, colour, rollouts);
        return chaos_node->select_move(colour);
    }

    void register_chaos_move(const Board::ChaosMove &move) override {
        board.place_chip(move);
        chip_pool = ChipPool(chip_pool, move.colour);

        if (chaos_node) {
            auto ptr = chaos_node->get_child(move);

            if (ptr) order_node = std::move(*ptr);
            else order_node = nullptr;

            chaos_node = nullptr;
        }
    }

    void register_order_move(const Board::OrderMove &move) override {
        board.move_chip(move);

        if (order_node) {
            auto ptr = order_node->get_child(move);

            if (ptr) chaos_node = std::move(*ptr);
            else chaos_node = nullptr;

            order_node = nullptr;
        }
    }

private:
    Board board;
    ChipPool chip_pool;
    std::unique_ptr<OrderNode> order_node{};
    std::unique_ptr<ChaosNode> chaos_node{};

    uint rollouts = 500000;
};


}// namespace entropy::mcts
