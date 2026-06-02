#include "MCTSNode.h"
#include <cmath>
#include <limits>

namespace metasolver {

MCTSNode::MCTSNode(State* state, Action* action, MCTSNode* parent, int depth) :
    state(state), action(action), parent(parent), visits(0), total_value(0.0), depth(depth), terminal(false) {
}

MCTSNode::~MCTSNode() {
    for(Action* action : untried_actions) delete action;
    for(MCTSNode* child : children) delete child;
    if(action) delete action;
    if(state) delete state;
}

bool MCTSNode::is_fully_expanded() const {
    return untried_actions.empty();
}

bool MCTSNode::is_terminal_node() const {
    return terminal;
}

double MCTSNode::uct_value(double exploration_constant) const {
    if(visits == 0) return std::numeric_limits<double>::infinity();
    double exploitation = total_value / double(visits);
    double exploration = exploration_constant * std::sqrt(std::log(double(parent->visits)) / double(visits));
    return exploitation + exploration;
}

MCTSNode* MCTSNode::best_child(double exploration_constant) const {
    MCTSNode* best = nullptr;
    double best_value = -std::numeric_limits<double>::infinity();
    for(MCTSNode* child : children){
        double value = child->uct_value(exploration_constant);
        if(value > best_value){
            best_value = value;
            best = child;
        }
    }
    return best;
}

void MCTSNode::add_child(MCTSNode* child) {
    children.push_back(child);
}

} /* namespace metasolver */
