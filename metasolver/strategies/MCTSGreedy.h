/*
 * MCTSGreedy.h
 *
 * Optional Monte Carlo reranker for candidates previously filtered by VCS.
 */

#ifndef STRATEGIES_MCTSGREEDY_H_
#define STRATEGIES_MCTSGREEDY_H_

#include "../SearchStrategy.h"
#include <vector>

namespace metasolver {

class MCTSGreedy {
public:
	struct Params {
		Params() :
			enabled(false),
			iterations(0),
			top_candidates(0),
			rollout_depth(10),
			rollout_width(3),
			exploration(1.4),
			vcs_weight(0.0),
			mcts_weight(1.0),
			complete_with_greedy(true) { }

		bool enabled;
		int iterations;
		int top_candidates;
		int rollout_depth;
		int rollout_width;
		double exploration;
		double vcs_weight;
		double mcts_weight;
		bool complete_with_greedy;
	};

	MCTSGreedy(ActionEvaluator* evl, SearchStrategy& greedy, const Params& params);

	void rerank_candidates(const State& state, list<Action*>& actions, double timelimit, clock_t begin_time);

	const Params& get_params() const { return params; }

private:
	struct CandidateStats {
		CandidateStats(Action* action, int vcs_rank, int n_candidates) :
			action(action), visits(0), total_reward(0.0), best_reward(0.0),
			vcs_score((double)(n_candidates - vcs_rank) / (double)n_candidates) { }

		Action* action;
		int visits;
		double total_reward;
		double best_reward;
		double vcs_score;
	};

	int select_candidate(const std::vector<CandidateStats>& candidates, int total_visits) const;
	double simulate(const State& state, const Action& first_action, double timelimit, clock_t begin_time);
	double get_time(clock_t begin_time) const;
	bool has_time(double timelimit, clock_t begin_time) const;
	void clear_actions(list<Action*>& actions);

	ActionEvaluator* evl;
	SearchStrategy& greedy;
	Params params;
};

} /* namespace metasolver */

#endif /* STRATEGIES_MCTSGREEDY_H_ */
