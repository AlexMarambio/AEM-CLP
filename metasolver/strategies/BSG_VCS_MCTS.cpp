#include "BSG_VCS_MCTS.h"
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include "../mcts/MCTS.h"

namespace metasolver {

BSG_VCS_MCTS::BSG_VCS_MCTS(ActionEvaluator* evl, SearchStrategy& greedy, const SolverConfig& config) :
    SearchStrategy(evl), greedy(greedy), config(config), nodes_explored(0) {
}

BSG_VCS_MCTS::~BSG_VCS_MCTS(){
}

list<State*> BSG_VCS_MCTS::next(list<State*>& S){
    if(S.empty()) return S;

    map<double, pair<State*, State*> > state_actions;

    int i = 0;
    for(list<State*>::iterator itS=S.begin(); itS!=S.end() && get_time()<=timelimit; itS++, i++){
        State& state = **itS;

        list<Action*> best_actions;
        int w = (double) config.max_level_size / (double) S.size() + 0.5;
        get_best_actions(state, best_actions, w);
        nodes_explored += best_actions.size();

        if(best_actions.empty()) continue;

        std::vector<Action*> best_actions_vector;
        best_actions_vector.reserve(best_actions.size());
        for(Action* action : best_actions){
            best_actions_vector.push_back(action);
        }

        int top_count = std::min((int)best_actions_vector.size(), config.top_k_vcs);
        std::vector<Action*> candidate_actions;
        candidate_actions.reserve(top_count);
        for(int index = 0; index < top_count; index++){
            candidate_actions.push_back(best_actions_vector[index]->clone());
        }

        MCTS mcts(evl, config);
        MCTSResult result = mcts.search_candidates(state, candidate_actions);
        last_mcts_report = result;

        for(int index = 0; index < top_count; index++){
            Action* action = best_actions_vector[index];
            State* state_copy = state.clone();
            state_copy->transition(*action);

            double value = greedy.run(*state_copy, timelimit, begin_time);
            if(value > get_best_value()){
                if(best_state) delete best_state;
                best_state = state_copy->clone();
                cout << "[BSG_VCS_MCTS] new best_solution_found ("<< get_time() <<"): " << value << " "
                     << best_state->get_path().size() << " nodes" << endl;
            }

            double score = (index < result.action_scores.size()) ? result.action_scores[index] : value;
            double key = -score;
            if(state_actions.find(key) == state_actions.end()){
                state_actions[key] = std::make_pair(&state, state_copy);
            } else {
                delete state_copy;
            }
        }

        for(int index = 0; index < (int)best_actions_vector.size(); index++){
            delete best_actions_vector[index];
        }
        for(Action* action : candidate_actions) delete action;
    }

    list<State*> nextS;
    int k = 0;
    for(auto it = state_actions.begin(); it != state_actions.end(); ++it){
        State* s = it->second.first;
        State* final_state = it->second.second;
        Action* a = (s)? s->next_action(*final_state):NULL;

        if(nextS.size() < (size_t) config.beam_width && a){
            State* p = s;
            s = s->clone();
            it->second.first = s;
            s->transition(*a);
            nextS.push_back(s);
            p->add_children(s);
        } else if(s){
            it->second.first = NULL;
        }

        if(k >= config.beam_width){
            delete final_state;
        }
        if(a) delete a;
        k++;
    }

    return nextS;
}

size_t BSG_VCS_MCTS::get_nodes_explored() const {
    return nodes_explored;
}

const MCTSResult& BSG_VCS_MCTS::get_last_mcts_report() const {
    return last_mcts_report;
}

} /* namespace metasolver */
