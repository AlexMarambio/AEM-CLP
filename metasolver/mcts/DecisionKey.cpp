#include "DecisionKey.h"

#include <map>
#include <sstream>

#include "../../problems/clp/clpState.h"
#include "../../problems/clp/objects2/BoxShape.h"

namespace metasolver {

std::string make_decision_key(const State& state, const Action& action) {
    const clp::clpAction* clp_action = dynamic_cast<const clp::clpAction*>(&action);
    if(!clp_action) {
        std::ostringstream fallback;
        fallback << "generic|depth=" << state.get_path().size();
        return fallback.str();
    }

    const clp::Block& block = clp_action->block;
    const clp::Space& space = clp_action->space;
    const clp::Vector3 location = space.get_location(block);
    std::ostringstream key;

    key << "clp"
        << "|depth=" << state.get_path().size()
        << "|block_dim=" << block.getL() << "x" << block.getW() << "x" << block.getH()
        << "|block_volume=" << block.getOccupiedVolume()
        << "|block_boxes=" << block.n_boxes;

    for(std::map<const clp::BoxShape*, int>::const_iterator it = block.nb_boxes.begin();
        it != block.nb_boxes.end(); ++it) {
        key << "|box=" << it->first->get_id() << ":" << it->second;
    }

    key << "|space_min=" << space.getXmin() << "," << space.getYmin() << "," << space.getZmin()
        << "|space_max=" << space.getXmax() << "," << space.getYmax() << "," << space.getZmax()
        << "|place=" << location.getX() << "," << location.getY() << "," << location.getZ();

    return key.str();
}

} /* namespace metasolver */
