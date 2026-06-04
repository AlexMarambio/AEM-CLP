#ifndef MCTS_DECISION_KEY_H_
#define MCTS_DECISION_KEY_H_

#include <string>
#include "../State.h"

namespace metasolver {

std::string make_decision_key(const State& state, const Action& action);

} /* namespace metasolver */

#endif /* MCTS_DECISION_KEY_H_ */
