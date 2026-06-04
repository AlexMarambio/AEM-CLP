#ifndef MCTS_STATS_H_
#define MCTS_STATS_H_

#include <map>
#include <string>

namespace metasolver {

struct MCTSActionStats {
    MCTSActionStats() : visits(0), total_reward(0.0), max_reward(0.0) {}

    int visits;
    double total_reward;
    double max_reward;
};

class MCTSStats {
public:
    MCTSStats();

    void update(const std::string& key, double reward);
    bool find(const std::string& key, MCTSActionStats& stats) const;
    double mean_value(const std::string& key) const;

    int size() const;
    int total_updates() const;
    double average_reward() const;

private:
    std::map<std::string, MCTSActionStats> action_stats;
    int update_count;
    double reward_sum;
};

} /* namespace metasolver */

#endif /* MCTS_STATS_H_ */
