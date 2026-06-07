/*
 * MCTSGreedy.cpp
 */

#include "MCTSGreedy.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>

namespace metasolver {

MCTSGreedy::MCTSGreedy(ActionEvaluator* evl, SearchStrategy& greedy, const Params& params) :
	evl(evl), greedy(greedy), params(params) { }

void MCTSGreedy::rerank_candidates(const State& state, list<Action*>& actions, double timelimit, clock_t begin_time){
	if(!params.enabled || params.iterations<=0 || actions.size()<2) return;

	std::vector<CandidateStats> candidates;
	list<Action*> tail_actions;
	int n_candidates = actions.size();
	if(params.top_candidates>0 && params.top_candidates<n_candidates)
		n_candidates = params.top_candidates;

	int rank = 0;
	for(list<Action*>::iterator action=actions.begin(); action!=actions.end(); action++, rank++){
		if(rank<n_candidates)
			candidates.push_back(CandidateStats(*action, rank, n_candidates));
		else
			tail_actions.push_back(*action);
	}

	int total_visits = 0;
	for(int iteration=0; iteration<params.iterations && has_time(timelimit, begin_time); iteration++){
		int selected = select_candidate(candidates, total_visits);
		double reward = simulate(state, *candidates[selected].action, timelimit, begin_time);

		candidates[selected].visits++;
		candidates[selected].total_reward += reward;
		if(reward > candidates[selected].best_reward)
			candidates[selected].best_reward = reward;
		total_visits++;
	}

	if(total_visits==0) return;

	std::stable_sort(candidates.begin(), candidates.end(),
		[this](const CandidateStats& a, const CandidateStats& b) {
			double avg_a = (a.visits>0)? a.total_reward / (double)a.visits : 0.0;
			double avg_b = (b.visits>0)? b.total_reward / (double)b.visits : 0.0;
			double score_a = params.mcts_weight * avg_a + params.vcs_weight * a.vcs_score;
			double score_b = params.mcts_weight * avg_b + params.vcs_weight * b.vcs_score;

			if(score_a != score_b) return score_a > score_b;
			if(a.best_reward != b.best_reward) return a.best_reward > b.best_reward;
			return a.vcs_score > b.vcs_score;
		});

	actions.clear();
	for(std::vector<CandidateStats>::iterator candidate=candidates.begin(); candidate!=candidates.end(); candidate++)
		actions.push_back(candidate->action);
	while(!tail_actions.empty()){
		actions.push_back(tail_actions.front());
		tail_actions.pop_front();
	}
}

int MCTSGreedy::select_candidate(const std::vector<CandidateStats>& candidates, int total_visits) const{
	for(size_t i=0; i<candidates.size(); i++)
		if(candidates[i].visits==0)
			return i;

	int selected = 0;
	double best_score = -1.0;
	double parent_visits = std::max(1, total_visits);

	for(size_t i=0; i<candidates.size(); i++){
		double avg = candidates[i].total_reward / (double)candidates[i].visits;
		double explore = params.exploration * std::sqrt(std::log(parent_visits) / (double)candidates[i].visits);
		double score = avg + explore;

		if(score > best_score){
			best_score = score;
			selected = i;
		}
	}

	return selected;
}

double MCTSGreedy::simulate(const State& state, const Action& first_action, double timelimit, clock_t begin_time){
	State* simulation = state.clone();
	simulation->transition(first_action);

	for(int depth=0; depth<params.rollout_depth && has_time(timelimit, begin_time); depth++){
		list<Action*> rollout_actions;
		int width = std::max(1, params.rollout_width);
		greedy.get_best_actions(*simulation, rollout_actions, width);

		if(rollout_actions.empty())
			break;

		int selected = rand() % rollout_actions.size();
		list<Action*>::iterator action = rollout_actions.begin();
		std::advance(action, selected);

		simulation->transition(**action);
		clear_actions(rollout_actions);
	}

	double reward = simulation->get_value();
	if(params.complete_with_greedy && has_time(timelimit, begin_time))
		reward = greedy.run(*simulation, timelimit, begin_time);

	delete simulation;
	return reward;
}

double MCTSGreedy::get_time(clock_t begin_time) const{
	return (double(clock()-begin_time)/double(CLOCKS_PER_SEC));
}

bool MCTSGreedy::has_time(double timelimit, clock_t begin_time) const{
	return timelimit==0.0 || get_time(begin_time)<=timelimit;
}

void MCTSGreedy::clear_actions(list<Action*>& actions){
	while(!actions.empty()){
		delete actions.front();
		actions.pop_front();
	}
}

} /* namespace metasolver */
