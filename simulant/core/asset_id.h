#pragma once

#include <cstdint>
#include "../assets/type.h"

namespace smlt {

typedef uint64_t AssetID;

bool asset_id_matches_type(AssetID id, AssetType type);
AssetID new_asset_id(AssetType type);

}
