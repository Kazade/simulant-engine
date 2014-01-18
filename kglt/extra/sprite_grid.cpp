#include "sprite_grid.h"
#include "../window_base.h"

#include "tiled/TmxParser/Tmx.h"

namespace kglt {
namespace extra {

TileInfo& GridChunk::tile(int32_t x_pos, int32_t y_pos) {
    return tiles_.at((y_pos * 16) + x_pos);
}

SpriteGrid::ptr SpriteGrid::new_from_file(Scene& scene, StageID stage, const unicode& filename, const unicode& layer) {
    Tmx::Map map;

    map.ParseFile(scene.window().resource_locator().locate_file(filename).encode());

    auto new_grid = SpriteGrid::create(scene, stage, map.GetWidth(), map.GetHeight());

    //TODO: Load the actual map in'it

    return new_grid;
}

SpriteGrid::SpriteGrid(Scene &scene, StageID stage, int32_t tiles_wide, int32_t tiles_high):
    scene_(scene),
    stage_id_(stage) {

    if((tiles_wide % 16) != 0 || (tiles_high % 16) != 0) {
        throw LogicError("SpriteGrid width and height must be divisible by 16");
    }

    tiles_wide_ = tiles_wide;
    tiles_high_ = tiles_high;

    chunks_wide_ = tiles_wide_ / 16;
    chunks_high_ = tiles_high_ / 16;

    chunks_.resize(chunks_wide_ * chunks_high_);

    for(uint32_t i = 0; i < chunks_.size(); ++i) {
        chunks_[i] = GridChunk::create();
    }
}

bool SpriteGrid::init() {
    return true;
}

void SpriteGrid::cleanup() {

}

GridChunk::ptr SpriteGrid::chunk(int32_t x_pos, int32_t y_pos) {
    return chunks_.at((y_pos * chunks_wide_) + x_pos);
}

TileInfo& SpriteGrid::tile_info(int32_t x_pos, int32_t y_pos) {
    int32_t chunk_x = x_pos / 16;
    int32_t chunk_y = y_pos / 16;

    return chunk(chunk_x, chunk_y)->tile(x_pos % 16, y_pos % 16);
}

}
}
