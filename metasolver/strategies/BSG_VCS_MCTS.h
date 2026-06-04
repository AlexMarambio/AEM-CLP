#ifndef STRATEGIES_BSG_VCS_MCTS_H_
#define STRATEGIES_BSG_VCS_MCTS_H_

#include "BSG.h"
#include "../SolverConfig.h"
#include "../mcts/MCTS.h"
#include "../mcts/MCTSStats.h"

namespace metasolver {

class BSG_VCS_MCTS : public SearchStrategy {
public:
    BSG_VCS_MCTS(ActionEvaluator* evl, SearchStrategy& greedy, const SolverConfig& config);
    virtual ~BSG_VCS_MCTS();

    virtual list<State*> next(list<State*>& S);
    size_t get_nodes_explored() const;
    const MCTSResult& get_last_mcts_report() const;

protected:
    template<class map_container>
    list<State*> get_next_states(map_container& sorted_states);

private:
    SearchStrategy& greedy;
    SolverConfig config;
    size_t nodes_explored;
    MCTSResult last_mcts_report;
    MCTSStats mcts_stats;
};

} /* namespace metasolver */

#endif /* STRATEGIES_BSG_VCS_MCTS_H_ */
