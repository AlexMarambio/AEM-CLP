/*
 * BSGpath.cpp
 *
 *  Created on: 12 jul. 2017
 *      Author: iaraya
 */

#include "BSG.h"

namespace metasolver {



BSG::~BSG(){
	clear_beam_stack();
	if(best_state) {
		delete best_state;
		best_state = NULL;
	}
	if(mcts) {
		delete mcts;
		mcts = NULL;
	}
}

list<State*> BSG::next(list<State*>& S){
	if(S.size()==0) return resume_from_beam_stack();

	CandidateMap ranked_candidates;

	int i=0;
	for(list<State*>::iterator itS=S.begin(); itS!=S.end() && get_time()<=timelimit; itS++,i++){
		State& state=**itS;

		if(state.is_root())
			cout << "beams/max_level_size:" << beams << "/" << max_level_size << endl;

		list< Action* > best_actions;
		int w =  (double) max_level_size / (double) S.size() + 0.5;
		get_best_actions(state, best_actions, w);
		if(mcts)
			mcts->rerank_candidates(state, best_actions, timelimit, begin_time);

		list< Action* >::iterator it = best_actions.begin();
		for(; it!=best_actions.end() && get_time()<=timelimit; it++){
			State* base_state = state.clone();
			State* final_state = state.clone();
			final_state->transition(**it);

			delete *it;

			double value = greedy.run(*final_state, timelimit, begin_time);

			if(value > get_best_value()){
				if(best_state) delete best_state;
				best_state = final_state->clone();
				cout << "[BSS] new best_solution_found ("<< get_time() <<"): " << value << " "
						<< best_state->get_path().size() << " nodes" << endl;
			}

			ranked_candidates.insert(make_pair(-value, new Candidate(value, base_state, final_state)));
		}
	}

	if(ranked_candidates.empty())
		return resume_from_beam_stack();

	list<State*> nextS;
	CandidateMap::iterator candidate = ranked_candidates.begin();
	int k=0;
	for(; candidate!=ranked_candidates.end() && k<beams; candidate++, k++){
		State* promoted = promote_candidate(candidate->second);
		if(promoted) nextS.push_back(promoted);
	}

	save_deferred_candidates(ranked_candidates, candidate);

	if(nextS.empty())
		return resume_from_beam_stack();

	return nextS;
}

State* BSG::promote_candidate(Candidate* candidate){
	State* next_state = NULL;
	Action* action = candidate->base_state->next_action(*candidate->final_state);

	if(action){
		next_state = candidate->base_state->clone();
		next_state->transition(*action);
		candidate->base_state->add_children(next_state);
		delete action;
	}

	delete_candidate(candidate);
	return next_state;
}

list<State*> BSG::resume_from_beam_stack(){
	list<State*> nextS;

	while(!beam_stack.empty() && nextS.empty()){
		BeamFrame& frame = beam_stack.back();
		//cout << "resuming from beam stack, deferred candidates: " << frame.deferred.size() << endl;
		while(!frame.deferred.empty() && nextS.size()<beams){
			Candidate* candidate = frame.deferred.front();
			frame.deferred.pop_front();

			State* promoted = promote_candidate(candidate);
			if(promoted) nextS.push_back(promoted);
		}

		if(frame.deferred.empty())
			beam_stack.pop_back();
	}

	return nextS;
}

void BSG::save_deferred_candidates(CandidateMap& ranked_candidates, CandidateMap::iterator first){
	BeamFrame frame;

	for(CandidateMap::iterator candidate=first; candidate!=ranked_candidates.end(); candidate++)
		frame.deferred.push_back(candidate->second);

	if(!frame.deferred.empty())
		beam_stack.push_back(frame);
}

void BSG::delete_candidate(Candidate* candidate){
	if(!candidate) return;
	if(candidate->base_state) delete candidate->base_state;
	if(candidate->final_state) delete candidate->final_state;
	delete candidate;
}

void BSG::clear_candidates(list<Candidate*>& candidates){
	while(!candidates.empty()){
		delete_candidate(candidates.front());
		candidates.pop_front();
	}
}

void BSG::clear_beam_stack(){
	while(!beam_stack.empty()){
		clear_candidates(beam_stack.back().deferred);
		beam_stack.pop_back();
	}
}

} /* namespace clp */
