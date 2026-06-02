#ifndef SOLVER_CONFIG_H_
#define SOLVER_CONFIG_H_

namespace metasolver {

struct SolverConfig {
    int beam_width;
    int max_level_size;
    int top_k_vcs;
    int mcts_iterations;
    int mcts_depth_limit;
    double mcts_time_limit_seconds;
    double exploration_constant;
    double vcs_rollout_bias;
    double reward_weight_fill_rate;
    double reward_weight_compactness;
    double reward_weight_contact_surface;
    double reward_weight_fragmentation;

    SolverConfig() :
        beam_width(4),
        max_level_size(16),
        top_k_vcs(10),
        mcts_iterations(100),
        mcts_depth_limit(6),
        mcts_time_limit_seconds(1.0),
        exploration_constant(1.4142135623730951),
        vcs_rollout_bias(0.8),
        reward_weight_fill_rate(1.0),
        reward_weight_compactness(0.5),
        reward_weight_contact_surface(0.5),
        reward_weight_fragmentation(0.5)
    {}
};

} /* namespace metasolver */

#endif /* SOLVER_CONFIG_H_ */
