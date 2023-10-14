#include <cstdint>
#include <limits>

#include "asset_id.h"
#include "../utils/random.h"

namespace smlt {

static RandomGenerator rgen;

bool asset_id_matches_type(AssetID id, AssetType type) {
    return ((id >> 32) & 0xF) == type;
}

AssetID new_asset_id(AssetType type) {
    return uint64_t(type) << 32 | rgen.int_in_range(1, std::numeric_limits<int32_t>::max() - 1);
}

}
