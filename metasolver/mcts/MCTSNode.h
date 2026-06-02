#ifndef MCTS_NODE_H_
#define MCTS_NODE_H_

#include <vector>
#include "../State.h"

namespace metasolver {

class MCTSNode {
public:
    MCTSNode(State* state, Action* action, MCTSNode* parent, int depth);
    virtual ~MCTSNode();

    bool is_fully_expanded() const;
    bool is_terminal_node() const;
    double uct_value(double exploration_constant) const;
    MCTSNode* best_child(double exploration_constant) const;
    void add_child(MCTSNode* child);

    State* state;
    Action* action;
    MCTSNode* parent;
    std::vector<MCTSNode*> children;
    std::vector<Action*> untried_actions;
    int visits;
    double total_value;
    int depth;
    bool terminal;
};

} /* namespace metasolver */

#endif /* MCTS_NODE_H_ */
