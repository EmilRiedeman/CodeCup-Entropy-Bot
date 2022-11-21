#pragma once

#include "board.hpp"
#include "move_maker.hpp"

#include <memory>

namespace entropy::mcts {

extern FastRand RNG;

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
    ChaosNode *const parent{};
    std::vector<std::unique_ptr<ChaosNode>> children{};

    const Board::ChaosMove last_move{};

    uint total_visits{};
    uint total_score{};

    std::array<Board::OrderMove::Compact, BOARD_AREA * 2> moves{};// not safe size
    uint unvisited{};

    friend ChaosNode;
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

    OrderNode *add_random_child(Colour colour);

    OrderNode *select_child(Colour colour) const;

    Board::ChaosMove select_move(Colour colour) const;

    void rollout() { record_score(rollout_board(board)); }

    [[nodiscard]] bool can_add_child(Colour colour) const { return unvisited[colour - 1]; }

    [[nodiscard]] float avg_score() const { return float(total_score) / 80 / float(total_visits); }

    [[nodiscard]] bool is_terminal() const { return !board.get_open_cells(); }

    Colour random_colour() const { return pool.random_chip(RNG); }

private:
    void init();

    void record_score(uint score);

    Board board;
    const ChipPool pool;
    OrderNode *const parent{};
    std::array<std::vector<std::unique_ptr<OrderNode>>, ChipPool::N> children{};

    const Board::OrderMove last_move{};

    std::array<uint, ChipPool::N> visits{};
    uint total_visits{};
    std::array<uint, ChipPool::N> scores{};
    uint total_score{};

    std::array<uint8_t, BOARD_AREA> moves{};
    std::array<uint8_t, ChipPool::N> unvisited{};

    friend OrderNode;
};

class MMChaos final : public ChaosMoveMaker {
public:
    MMChaos() {
        std::cerr << "MCTS Seed: " << RNG.seed << '\n';
    }

    Board::ChaosMove suggest_move(Colour colour) override {
        ChaosNode node(board, chip_pool);
        tree_search_chaos(node, colour, rollouts);
        return node.select_move(colour);
    }

    void register_chaos_move(const Board::ChaosMove &move) override {
        board.place_chip(move);
        chip_pool = ChipPool(chip_pool, move.colour);
    }

    void register_order_move(const Board::OrderMove &move) override {
        board.move_chip(move);
    }

private:
    Board board;
    ChipPool chip_pool;

    uint rollouts = 50000;
};


}// namespace entropy::mcts
