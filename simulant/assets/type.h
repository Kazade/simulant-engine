#pragma once

#include <cstdint>

namespace smlt {

enum AssetType : uint8_t {
    ASSET_TYPE_MESH,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_PARTICLE_SCRIPT,
    ASSET_TYPE_FONT,
    ASSET_TYPE_VIDEO,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_BINARY
};

}
