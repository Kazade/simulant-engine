#pragma once

#include <cstdint>

namespace smlt {

typedef uint64_t StageNodeID;

bool stage_node_id_matches_type(StageNodeID id, uint16_t node_type);
StageNodeID new_stage_node_id(uint16_t node_type);

}
