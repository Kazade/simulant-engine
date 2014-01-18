#ifndef SPRITE_GRID_H
#define SPRITE_GRID_H

#include <kazbase/unicode.h>

#include "../base.h"
#include "../generic/managed.h"

namespace kglt {
namespace extra {

//Represents the information about a single tile
struct TileInfo {

};

/*
 *  A chunk is a 16x16 grid of tiles, represented by a single actor/mesh
 *  this is for culling reasons and is not exposed by the API aside from
 *  the requirement that the grid size must be divisible by 16
 */
class GridChunk:
    public Managed<GridChunk> {

public:
    TileInfo& tile(int32_t x_pos, int32_t y_pos);

private:
    ActorID actor_;

    std::vector<TileInfo> tiles_;
};

/*
 *  A sprite grid is an array of sprite chunks
 */
class SpriteGrid:
    public Managed<SpriteGrid> {

public:
    static SpriteGrid::ptr new_from_file(Scene& scene, StageID, const unicode& filename, const unicode& layer);
    SpriteGrid(Scene& scene, StageID stage, int32_t tiles_wide, int32_t tiles_high);

    bool init();
    void cleanup();

    TileInfo& tile_info(int32_t x_pos, int32_t y_pos);

private:
    Scene& scene_;
    StageID stage_id_;

    int32_t tiles_wide_;
    int32_t tiles_high_;
    int32_t chunks_wide_;
    int32_t chunks_high_;

    GridChunk::ptr chunk(int32_t x_pos, int32_t y_pos);

    std::vector<GridChunk::ptr> chunks_;
};

}
}

#endif
