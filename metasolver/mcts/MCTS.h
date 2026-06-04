#ifndef MCTS_H_
#define MCTS_H_

#include <vector>
#include <ctime>
#include "../ActionEvaluator.h"
#include "../State.h"
#include "../SolverConfig.h"
#include "MCTSNode.h"
#include "MCTSStats.h"

namespace clp {
class AABB;
class Block;
}

namespace metasolver {

struct MCTSResult {
    std::vector<double> action_scores;
    int iterations;
    int nodes_explored;
    int simulation_count;
    int max_depth;
    double average_depth;
    double total_simulation_time;
    double average_reward;
    double max_reward;
    int global_stats_size;
    int global_stats_updates;
    double global_average_reward;
    int global_score_hits;
    double average_global_score;
};

class MCTS {
public:
    MCTS(ActionEvaluator* evaluator, const SolverConfig& config, MCTSStats* global_stats = nullptr);
    virtual ~MCTS();

    MCTSResult search_candidates(const State& root_state, const std::vector<Action*>& initial_actions);

private:
    MCTSNode* tree_policy(MCTSNode* node);
    MCTSNode* expand(MCTSNode* node);
    double default_policy(MCTSNode* node);
    void backpropagate(MCTSNode* node, double reward);
    void fill_global_stats_report(MCTSResult& report) const;
    std::vector<Action*> get_best_actions(const State& state, int n);
    void initialize_untried_actions(MCTSNode* node);
    bool is_terminal(const State& state, int depth) const;
    Action* select_rollout_action(const State& state, const std::vector<Action*>& actions) const;
    double compute_reward(const State& state) const;
    double compute_fill_rate(const State& state) const;
    double compute_compactness(const State& state) const;
    double compute_contact_surface(const State& state) const;
    long overlap_area(const clp::AABB& a, const clp::AABB& b) const;
    long compute_contact_surface(const clp::AABB& block, const clp::Block& container) const;
    double compute_fragmentation(const State& state) const;

private:
    ActionEvaluator* evaluator;
    SolverConfig config;
    MCTSStats* global_stats;
    int nodes_explored;
    int rollout_depth;
};

} /* namespace metasolver */

#endif /* MCTS_H_ */
