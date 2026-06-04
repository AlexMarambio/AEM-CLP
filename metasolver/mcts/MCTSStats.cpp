#include "MCTSStats.h"

namespace metasolver {

MCTSStats::MCTSStats() : update_count(0), reward_sum(0.0) {
}

void MCTSStats::update(const std::string& key, double reward) {
    MCTSActionStats& stats = action_stats[key];
    stats.visits++;
    stats.total_reward += reward;
    if(stats.visits == 1 || reward > stats.max_reward) stats.max_reward = reward;
    update_count++;
    reward_sum += reward;
}

bool MCTSStats::find(const std::string& key, MCTSActionStats& stats) const {
    std::map<std::string, MCTSActionStats>::const_iterator it = action_stats.find(key);
    if(it == action_stats.end()) return false;
    stats = it->second;
    return true;
}

double MCTSStats::mean_value(const std::string& key) const {
    MCTSActionStats stats;
    if(!find(key, stats) || stats.visits == 0) return 0.0;
    return stats.total_reward / double(stats.visits);
}

int MCTSStats::size() const {
    return action_stats.size();
}

int MCTSStats::total_updates() const {
    return update_count;
}

double MCTSStats::average_reward() const {
    return update_count > 0 ? reward_sum / double(update_count) : 0.0;
}

} /* namespace metasolver */
