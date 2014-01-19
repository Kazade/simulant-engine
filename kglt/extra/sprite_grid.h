#ifndef SPRITE_GRID_H
#define SPRITE_GRID_H

#include <kazbase/unicode.h>

#include "../base.h"
#include "../generic/managed.h"

namespace kglt {
namespace extra {

//Represents the information about a single tile
struct TileInfo {
    SubMeshIndex submesh;
};

class SpriteGrid;

/*
 *  A chunk is a 16x16 grid of tiles, represented by a single actor/mesh
 *  this is for culling reasons and is not exposed by the API aside from
 *  the requirement that the grid size must be divisible by 16
 */
class GridChunk:
    public Managed<GridChunk> {

public:
    GridChunk(SpriteGrid* parent);
    TileInfo& tile(int32_t x_pos, int32_t y_pos);

    bool init();
    void cleanup();

    void set_tile_texture_info(
        int32_t tile_index,
        TextureID texture_id,
        float x0, float y0, float x1, float y1
    );

private:
    SpriteGrid* parent_;
    ActorID actor_id_;
    MeshID mesh_id_;

    std::vector<TileInfo> tiles_;
};

/*
 *  A sprite grid is an array of sprite chunks
 */

class SpriteGrid:
    public Managed<SpriteGrid> {

public:
    static SpriteGrid::ptr new_from_file(Scene& scene, StageID, const unicode& filename, const unicode& layer);
    SpriteGrid(Scene& scene, StageID stage, int32_t tiles_wide, int32_t tiles_high, float tile_render_size=1.0);

    bool init();
    void cleanup();

    TileInfo& tile_info(int32_t x_pos, int32_t y_pos);

    void add_tileset(int32_t tileset_id, TextureID tileset) { tilesets_[tileset_id] = tileset; }

    GridChunk::ptr chunk(int32_t tile_x_pos, int32_t tile_y_pos);
    GridChunk::ptr chunk_at(int32_t index) { return chunks_.at(index); }
    /** Given a global tile position, return the index inside the tiles chunk that
     *  the tile appears.
     */
    std::pair<GridChunk::ptr, uint32_t> chunk_tile_index(int32_t tile_x_pos, int32_t tile_y_pos);

private:
    Stage& stage();

    Scene& scene_;
    StageID stage_id_;

    int32_t tiles_wide_;
    int32_t tiles_high_;
    int32_t chunks_wide_;
    int32_t chunks_high_;
    float tile_render_size_;

    std::vector<GridChunk::ptr> chunks_;
    std::map<int32_t, TextureID> tilesets_;

    friend class GridChunk;
    friend class SpriteGridTests;
};

}
}

#endif
