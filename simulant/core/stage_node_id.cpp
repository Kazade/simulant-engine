#include "stage_node_id.h"
#include "../utils/random.h"

namespace smlt {

static RandomGenerator rgen;

bool stage_node_id_matches_type(StageNodeID id, uint16_t node_type) {
    return ((id >> 32) & 0xFF) == node_type;
}

StageNodeID new_stage_node_id(uint16_t node_type) {
    return StageNodeID(node_type) << 32 | rgen.int_in_range(1, std::numeric_limits<int32_t>::max() - 1);
}

}
