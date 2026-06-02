#include "MCTS.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>

#include "../../problems/clp/clpState.h"
#include "../../problems/clp/objects2/AABB.h"
#include "../../problems/clp/objects2/Block.h"
#include "../../problems/clp/objects2/Space.h"
#include "../../problems/clp/objects2/SpaceSet.h"

namespace metasolver {

MCTS::MCTS(ActionEvaluator* evaluator, const SolverConfig& config) :
    evaluator(evaluator), config(config), nodes_explored(0) {
}

MCTS::~MCTS() {
}

MCTSResult MCTS::search_candidates(const State& root_state, const std::vector<Action*>& initial_actions) {
    MCTSResult report;
    report.iterations = 0;
    report.nodes_explored = 0;
    report.simulation_count = 0;
    report.max_depth = 0;
    report.average_depth = 0.0;
    report.total_simulation_time = 0.0;
    report.average_reward = 0.0;
    report.max_reward = 0.0;

    MCTSNode* root = new MCTSNode(root_state.clone(), nullptr, nullptr, 0);
    root->terminal = false;
    nodes_explored = 0;

    for(Action* action : initial_actions){
        State* child_state = root->state->clone();
        child_state->transition(*action);
        Action* action_clone = action->clone();
        MCTSNode* child = new MCTSNode(child_state, action_clone, root, 1);
        initialize_untried_actions(child);
        root->add_child(child);
        nodes_explored++;
    }

    if(root->children.empty()){
        report.action_scores.clear();
        report.iterations = 0;
        report.nodes_explored = nodes_explored;
        delete root;
        return report;
    }

    clock_t begin = clock();
    int total_depth = 0;
    double total_reward = 0.0;
    double max_reward = -1e9;
    int simulation_count = 0;
    int max_depth = 0;

    for(int iteration = 0; iteration < config.mcts_iterations; iteration++){
        double elapsed = double(clock() - begin) / double(CLOCKS_PER_SEC);
        if(config.mcts_time_limit_seconds > 0.0 && elapsed >= config.mcts_time_limit_seconds) break;

        MCTSNode* node = tree_policy(root);
        double reward = default_policy(node);
        backpropagate(node, reward);

        simulation_count++;
        total_reward += reward;
        if(reward > max_reward) max_reward = reward;
        if(node->depth > max_depth) max_depth = node->depth;
        total_depth += rollout_depth;
    }

    report.iterations = simulation_count;
    report.nodes_explored = nodes_explored;
    report.simulation_count = simulation_count;
    report.max_depth = max_depth;
    report.average_depth = simulation_count > 0 ? double(total_depth) / double(simulation_count) : 0.0;
    report.total_simulation_time = double(clock() - begin) / double(CLOCKS_PER_SEC);
    report.average_reward = simulation_count > 0 ? total_reward / double(simulation_count) : 0.0;
    report.max_reward = max_reward < -1e8 ? 0.0 : max_reward;

    report.action_scores.reserve(root->children.size());
    for(MCTSNode* child : root->children){
        if(child->visits > 0){
            report.action_scores.push_back(child->total_value / double(child->visits));
        } else {
            report.action_scores.push_back(compute_reward(*child->state));
        }
    }

    delete root;
    return report;
}

MCTSNode* MCTS::tree_policy(MCTSNode* node){
    while(!node->terminal){
        if(!node->is_fully_expanded()) return expand(node);
        else node = node->best_child(config.exploration_constant);
    }
    return node;
}

MCTSNode* MCTS::expand(MCTSNode* node){
    if(node->untried_actions.empty()){
        node->terminal = true;
        return node;
    }

    Action* action = node->untried_actions.back();
    node->untried_actions.pop_back();
    State* child_state = node->state->clone();
    child_state->transition(*action);
    Action* cloned_action = action->clone();
    MCTSNode* child = new MCTSNode(child_state, cloned_action, node, node->depth + 1);
    child->terminal = is_terminal(*child_state, child->depth);
    if(!child->terminal) initialize_untried_actions(child);
    node->add_child(child);
    nodes_explored++;
    return child;
}

double MCTS::default_policy(MCTSNode* node){
    State* state = node->state->clone();
    int depth = node->depth;
    double reward = 0.0;
    rollout_depth = 0;

    while(true){
        if(is_terminal(*state, depth)) break;
        std::vector<Action*> actions = get_best_actions(*state, config.top_k_vcs);
        if(actions.empty()) break;
        Action* selected = select_rollout_action(*state, actions);
        if(!selected) break;
        state->transition(*selected);
        for(Action* a : actions) if(a != selected) delete a;
        delete selected;
        depth++;
        rollout_depth++;
    }

    reward = compute_reward(*state);
    delete state;
    return reward;
}

void MCTS::backpropagate(MCTSNode* node, double reward){
    while(node != nullptr){
        node->visits++;
        node->total_value += reward;
        node = node->parent;
    }
}

std::vector<Action*> MCTS::get_best_actions(const State& state, int n){
    if(!evaluator){
        std::cout << "MCTS needs an ActionEvaluator" << std::endl;
        exit(0);
    }

    std::multimap<double, Action*> ranked_actions;
    std::list<Action*> actions;
    state.get_actions(actions);

    while(!actions.empty()){
        Action* a = actions.front();
        actions.pop_front();
        double eval = evaluator->eval_action_rand(state, *a);
        if(eval > 0.0 && ((int)ranked_actions.size() < n || (!ranked_actions.empty() && ranked_actions.begin()->first < eval))){
            ranked_actions.insert(std::make_pair(eval, a));
            if((int)ranked_actions.size() == n + 1){
                delete ranked_actions.begin()->second;
                ranked_actions.erase(ranked_actions.begin());
            }
        } else {
            delete a;
        }
    }

    std::vector<Action*> best_actions;
    best_actions.reserve(ranked_actions.size());
    for(auto it = ranked_actions.rbegin(); it != ranked_actions.rend(); ++it){
        best_actions.push_back(it->second);
    }

    return best_actions;
}

void MCTS::initialize_untried_actions(MCTSNode* node){
    if(node->terminal) return;
    std::vector<Action*> actions = get_best_actions(*node->state, config.top_k_vcs);
    for(Action* action : actions){
        node->untried_actions.push_back(action);
    }
    if(node->untried_actions.empty()) node->terminal = true;
}

bool MCTS::is_terminal(const State& state, int depth) const{
    if(depth >= config.mcts_depth_limit) return true;
    std::list<Action*> actions;
    state.get_actions(actions);
    bool terminal = actions.empty();
    while(!actions.empty()){ delete actions.front(); actions.pop_front(); }
    return terminal;
}

Action* MCTS::select_rollout_action(const State& state, const std::vector<Action*>& actions) const{
    if(actions.empty()) return nullptr;
    double roll = double(rand()) / double(RAND_MAX);
    if(roll <= config.vcs_rollout_bias){
        return actions.front()->clone();
    }
    int index = rand() % actions.size();
    return actions[index]->clone();
}

static double safe_div(double numerator, double denominator){
    return denominator == 0.0 ? 0.0 : numerator / denominator;
}

double MCTS::compute_reward(const State& state) const{
    double fill_rate = compute_fill_rate(state);
    double compactness = compute_compactness(state);
    double contact_surface = compute_contact_surface(state);
    double fragmentation = compute_fragmentation(state);
    return config.reward_weight_fill_rate * fill_rate
        + config.reward_weight_compactness * compactness
        + config.reward_weight_contact_surface * contact_surface
        - config.reward_weight_fragmentation * fragmentation;
}

double MCTS::compute_fill_rate(const State& state) const{
    return state.get_value();
}

double MCTS::compute_contact_surface(const State& state) const{
    const clp::clpState* clp_state = dynamic_cast<const clp::clpState*>(&state);
    if(!clp_state || !clp_state->cont) return 0.0;
    const clp::Block* container = clp_state->cont;
    long container_surface = 2 * (container->getL() * container->getW() + container->getL() * container->getH() + container->getW() * container->getH());
    if(container_surface == 0) return 0.0;

    long total_contact = 0;
    if(container->blocks && container->blocks->size() > 0){
        const clp::AABB* current = &container->blocks->top();
        while(true){
            total_contact += compute_contact_surface(*current, *container);
            std::list<const clp::AABB*> neighbors = container->blocks->get_intersected_objects(*current);
            for(const clp::AABB* neighbor : neighbors){
                if(neighbor != current){
                    total_contact += overlap_area(*current, *neighbor);
                }
            }
            if(!container->blocks->has_next()) break;
            current = &container->blocks->next();
        }
    }
    return safe_div((double) total_contact, (double) container_surface);
}

long MCTS::overlap_area(const clp::AABB& a, const clp::AABB& b) const{
    long overlap = 0;
    long x_min = std::max(a.getXmin(), b.getXmin());
    long x_max = std::min(a.getXmax(), b.getXmax());
    long y_min = std::max(a.getYmin(), b.getYmin());
    long y_max = std::min(a.getYmax(), b.getYmax());
    long z_min = std::max(a.getZmin(), b.getZmin());
    long z_max = std::min(a.getZmax(), b.getZmax());

    if(x_max == a.getXmin() && x_min == x_max){
        long area = std::max(0L, (y_max - y_min)) * std::max(0L, (z_max - z_min));
        overlap += area;
    }
    if(x_min == a.getXmax() && x_min == x_max){
        long area = std::max(0L, (y_max - y_min)) * std::max(0L, (z_max - z_min));
        overlap += area;
    }
    if(y_max == a.getYmin() && y_min == y_max){
        long area = std::max(0L, (x_max - x_min)) * std::max(0L, (z_max - z_min));
        overlap += area;
    }
    if(y_min == a.getYmax() && y_min == y_max){
        long area = std::max(0L, (x_max - x_min)) * std::max(0L, (z_max - z_min));
        overlap += area;
    }
    if(z_max == a.getZmin() && z_min == z_max){
        long area = std::max(0L, (x_max - x_min)) * std::max(0L, (y_max - y_min));
        overlap += area;
    }
    if(z_min == a.getZmax() && z_min == z_max){
        long area = std::max(0L, (x_max - x_min)) * std::max(0L, (y_max - y_min));
        overlap += area;
    }
    return overlap;
}

long MCTS::compute_contact_surface(const clp::AABB& block, const clp::Block& container) const{
    long surface = 0;
    if(block.getXmin() <= 0) surface += block.getW() * block.getH();
    if(block.getXmax() >= container.getL()) surface += block.getW() * block.getH();
    if(block.getYmin() <= 0) surface += block.getL() * block.getH();
    if(block.getYmax() >= container.getW()) surface += block.getL() * block.getH();
    if(block.getZmin() <= 0) surface += block.getL() * block.getW();
    if(block.getZmax() >= container.getH()) surface += block.getL() * block.getW();
    return surface;
}

double MCTS::compute_compactness(const State& state) const{
    return compute_contact_surface(state);
}

double MCTS::compute_fragmentation(const State& state) const{
    const clp::clpState* clp_state = dynamic_cast<const clp::clpState*>(&state);
    if(!clp_state || !clp_state->cont || !clp_state->cont->spaces) return 0.0;
    const clp::Block* container = clp_state->cont;
    int spaces = container->spaces->size();
    if(spaces == 0) return 0.0;

    double free_volume = 0.0;
    const clp::Space* space = &container->spaces->top();
    while(true){
        free_volume += space->getVolume();
        if(!container->spaces->has_next()) break;
        space = &container->spaces->next();
    }

    double free_fraction = safe_div(free_volume, container->getVolume());
    return free_fraction * (1.0 + double(spaces) / 10.0);
}

} /* namespace metasolver */
