#include "stage_node_id.h"
#include "../utils/random.h"

namespace smlt {

bool stage_node_id_matches_type(StageNodeID id, uint16_t node_type) {
    return ((id >> 16) & 0xFFFF) == node_type;
}

StageNodeID new_stage_node_id(uint16_t node_type) {
    return StageNodeID(node_type) << 16 |
           RandomGenerator::instance().int_in_range(
               1, std::numeric_limits<int16_t>::max() - 1);
}

}
