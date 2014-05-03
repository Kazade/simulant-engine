#include <kazbase/utils.h>

#include "../procedural/mesh.h"
#include "sprite_grid.h"
#include "../window_base.h"
#include "../actor.h"
#include "../stage.h"
#include "tiled/TmxParser/Tmx.h"

namespace kglt {
namespace extra {

static const uint32_t CHUNK_WIDTH_IN_TILES = 16;

GridChunk::GridChunk(SpriteGrid *parent, const Vec2& offset):
    parent_(parent),
    offset_(offset) {

    tiles_.resize(CHUNK_WIDTH_IN_TILES * CHUNK_WIDTH_IN_TILES);
}

bool GridChunk::init() {
    //Create a mesh for the chunk
    mesh_id_ = parent_->stage()->new_mesh();

    {
        auto mesh = parent_->stage()->mesh(mesh_id_);

        //Generate a rectangle submesh for all tiles in the chunk
        for(uint32_t y = 0; y < CHUNK_WIDTH_IN_TILES; ++y) {
            for(uint32_t x = 0; x < CHUNK_WIDTH_IN_TILES; ++x) {
                int32_t tile_info_index = (y * CHUNK_WIDTH_IN_TILES) + x;

                tiles_.at(tile_info_index).submesh = procedural::mesh::new_rectangle_submesh(
                    mesh,
                    parent_->tile_render_size_,
                    parent_->tile_render_size_,
                    x * parent_->tile_render_size_ + (parent_->tile_render_size_ / 2),
                    (CHUNK_WIDTH_IN_TILES - y) * parent_->tile_render_size_ - (parent_->tile_render_size_ / 2)
                );
            }
        }
    }

    actor_id_ = parent_->stage()->new_actor_with_parent_and_mesh(parent_->group_actor_id_, mesh_id_);
    parent_->stage()->actor(actor_id_)->move_to(offset_.x, offset_.y, 0);
    parent_->stage()->actor(actor_id_)->set_render_priority(RENDER_PRIORITY_BACKGROUND);
    return true;
}

void GridChunk::cleanup() {

}

void GridChunk::set_tile_texture_info(int32_t tile_index, TextureID texture_id, float x0, float y0, float x1, float y1) {
    auto mesh = parent_->stage()->mesh(mesh_id_);
    auto& submesh = mesh->submesh(tiles_.at(tile_index).submesh);
    submesh.set_texture_on_material(0, texture_id);

    submesh.vertex_data().move_to(0);
    submesh.vertex_data().tex_coord0(kglt::Vec2(x0, y1));

    submesh.vertex_data().move_next();
    submesh.vertex_data().tex_coord0(kglt::Vec2(x1, y1));

    submesh.vertex_data().move_next();
    submesh.vertex_data().tex_coord0(kglt::Vec2(x1, y0));

    submesh.vertex_data().move_next();
    submesh.vertex_data().tex_coord0(kglt::Vec2(x0, y0));

    submesh.vertex_data().done();
}

TileInfo& GridChunk::tile(int32_t x_pos, int32_t y_pos) {
    return tiles_.at((y_pos * CHUNK_WIDTH_IN_TILES) + x_pos);
}

struct TilesetInfo {
    uint32_t total_width;
    uint32_t total_height;

    uint32_t tile_width;
    uint32_t tile_height;

    uint32_t spacing;
    uint32_t margin;

    uint32_t num_tiles_wide() const {
        return (total_width - (margin * 2) + spacing) / (tile_width + spacing);
    }

    uint32_t num_tiles_high() const {
        return (total_height - (margin * 2) + spacing) / (tile_height + spacing);
    }
};

kglt::Vec2 SpriteGrid::pixel_to_world(const kglt::Vec2& position) {
    return Vec2(
        (position.x / float(map_tile_width_)) * tile_render_size_,
        (position.y / float(map_tile_height_)) * tile_render_size_
    );
}

SpriteGrid::ptr SpriteGrid::new_from_file(WindowBase& window, StageID stage, const unicode& filename, const unicode& layer_name, float tile_render_size) {
    Tmx::Map map;

    map.ParseFile(window.resource_locator().locate_file(filename).encode());

    auto layers = map.GetLayers();
    auto it = std::find_if(layers.begin(), layers.end(), [=](Tmx::Layer* layer) { return layer->GetName() == layer_name.encode(); });

    if(it == layers.end()) {
        throw RuntimeError(_u("Unable to find the layer with name: {0}").format(layer_name).encode());
    }

    Tmx::Layer* layer = (*it);

    SpriteGrid::ptr new_grid = SpriteGrid::create(window, stage, layer->GetWidth(), layer->GetHeight(), tile_render_size);

    unicode parent_dir = os::path::abs_path(os::path::dir_name(filename));

    std::map<int32_t, TilesetInfo> tileset_info;

    //Load all of the tilesets from the map
    for(int32_t i = 0; i < map.GetNumTilesets(); ++i) {
        const Tmx::Tileset* tileset = map.GetTileset(i);
        const Tmx::Image* image = tileset->GetImage();
        unicode rel_path = image->GetSource();

        unicode final_path = os::path::join(parent_dir, rel_path);
        L_DEBUG(_u("Loading tileset from: {0}").format(final_path));
        new_grid->add_tileset(i, new_grid->stage()->new_texture_from_file(
            final_path,
            TEXTURE_OPTION_CLAMP_TO_EDGE | TEXTURE_OPTION_DISABLE_MIPMAPS | TEXTURE_OPTION_NEAREST_FILTER
        ));

        TilesetInfo& info = tileset_info[i];
        info.margin = tileset->GetMargin();
        info.spacing = tileset->GetSpacing();
        info.tile_height = tileset->GetTileHeight();
        info.tile_width = tileset->GetTileWidth();
        info.total_height = image->GetHeight();
        info.total_width = image->GetWidth();
    }

    for(int32_t y = 0; y < layer->GetHeight(); ++y) {
        for(int32_t x = 0; x < layer->GetWidth(); ++x) {
            int32_t tileset_index = layer->GetTileTilesetIndex(x, y);

            TilesetInfo& tileset = tileset_info[tileset_index];

            int32_t tile_id = layer->GetTileId(x, y);

            int32_t num_tiles_wide = tileset.num_tiles_wide();

            int32_t x_offset = tile_id % num_tiles_wide;
            int32_t y_offset = tile_id / num_tiles_wide;

            float x0 = x_offset * (tileset.tile_width + tileset.spacing) + tileset.margin;
            float y0 = tileset.total_height - y_offset * (tileset.tile_height + tileset.spacing) - tileset.margin;

            float x1 = x0 + tileset.tile_width;
            float y1 = y0 - tileset.tile_height;

            auto chunk_and_index = new_grid->chunk_tile_index(x, y);

            chunk_and_index.first->set_tile_texture_info(
                chunk_and_index.second,
                new_grid->tilesets_[tileset_index],
                x0 / tileset.total_width + (0.5 / tileset.total_width),
                y0 / tileset.total_height - (0.5 / tileset.total_height),
                x1 / tileset.total_width - (0.5 / tileset.total_width),
                y1 / tileset.total_height + (0.5 / tileset.total_height)
            );
        }
    }

    new_grid->map_tile_height_ = map.GetTileHeight();
    new_grid->map_tile_width_ = map.GetTileWidth();


    return new_grid;
}

SpriteGrid::SpriteGrid(WindowBase& window, StageID stage, int32_t tiles_wide, int32_t tiles_high, float tile_render_size):
    window_(window),
    stage_id_(stage),
    tile_render_size_(tile_render_size) {

    if((tiles_wide % CHUNK_WIDTH_IN_TILES) != 0 || (tiles_high % CHUNK_WIDTH_IN_TILES) != 0) {
        throw LogicError(_u("SpriteGrid width and height must be divisible by {0}").format(CHUNK_WIDTH_IN_TILES).encode());
    }

    tiles_wide_ = tiles_wide;
    tiles_high_ = tiles_high;

    chunks_wide_ = tiles_wide_ / CHUNK_WIDTH_IN_TILES;
    chunks_high_ = tiles_high_ / CHUNK_WIDTH_IN_TILES;

    //Initialize the parent actor that all chunks are nested under
    group_actor_id_ = window.stage(stage)->new_actor();
}

Vec2 SpriteGrid::render_dimensions() const {
    return Vec2(
        chunks_wide_ * CHUNK_WIDTH_IN_TILES * tile_render_size_,
        chunks_high_ * CHUNK_WIDTH_IN_TILES * tile_render_size_
    );
}

bool SpriteGrid::init() {        
    chunks_.resize(chunks_wide_ * chunks_high_);

    for(uint32_t y = 0; y < chunks_high_; ++y) {
        for(uint32_t x = 0; x < chunks_wide_; ++x) {
            chunks_[(chunks_wide_ * y) + x] = GridChunk::create(
                this, Vec2(
                    x * CHUNK_WIDTH_IN_TILES * tile_render_size_,
                    y * CHUNK_WIDTH_IN_TILES * tile_render_size_
                )
            );
        }
    }

    return true;
}

void SpriteGrid::cleanup() {

}

StagePtr SpriteGrid::stage() const {
    return window_.stage(stage_id_);
}

StagePtr SpriteGrid::stage() {
    return window_.stage(stage_id_);
}

GridChunk::ptr SpriteGrid::chunk(int32_t tile_x_pos, int32_t tile_y_pos) {
    int32_t chunk_x = tile_x_pos / CHUNK_WIDTH_IN_TILES;
    int32_t chunk_y = tile_y_pos / CHUNK_WIDTH_IN_TILES;
    int32_t final_index = (chunk_y * chunks_wide_) + chunk_x;

    return chunks_.at(final_index);
}

std::pair<GridChunk::ptr, uint32_t> SpriteGrid::chunk_tile_index(int32_t tile_x_pos, int32_t tile_y_pos) {
    int32_t x = tile_x_pos % CHUNK_WIDTH_IN_TILES;
    int32_t y = tile_y_pos % CHUNK_WIDTH_IN_TILES;
    int32_t index = (y * CHUNK_WIDTH_IN_TILES) + x;
    return std::make_pair(chunk(tile_x_pos, tile_y_pos), index);
}

TileInfo& SpriteGrid::tile_info(int32_t x_pos, int32_t y_pos) {
    int32_t chunk_x = x_pos / CHUNK_WIDTH_IN_TILES;
    int32_t chunk_y = y_pos / CHUNK_WIDTH_IN_TILES;

    return chunk(chunk_x, chunk_y)->tile(x_pos % CHUNK_WIDTH_IN_TILES, y_pos % CHUNK_WIDTH_IN_TILES);
}

//Moveable interface
void SpriteGrid::move_to(const kglt::Vec3& pos) {
    stage()->actor(group_actor_id_)->move_to(pos);
}

void SpriteGrid::move_to(const kglt::Vec2& pos) {
    stage()->actor(group_actor_id_)->move_to(pos);
}

void SpriteGrid::move_to(float x, float y, float z) {
    stage()->actor(group_actor_id_)->move_to(x, y, z);
}

void SpriteGrid::move_to(float x, float y) {
    stage()->actor(group_actor_id_)->move_to(x, y);
}

void SpriteGrid::rotate_to(const kglt::Degrees& angle) {
    stage()->actor(group_actor_id_)->rotate_to(angle);
}

void SpriteGrid::rotate_to(const kglt::Degrees& angle, float axis_x, float axis_y, float axis_z) {
    stage()->actor(group_actor_id_)->rotate_to(angle, axis_x, axis_y, axis_z);
}

void SpriteGrid::rotate_to(const kglt::Degrees& angle, const kglt::Vec3& axis) {
    stage()->actor(group_actor_id_)->rotate_to(angle, axis);
}

void SpriteGrid::rotate_to(const kglt::Quaternion& rotation) {
    stage()->actor(group_actor_id_)->rotate_to(rotation);
}

void SpriteGrid::rotate_x(const Degrees &angle) {
    stage()->actor(group_actor_id_)->rotate_x(angle);
}

void SpriteGrid::rotate_y(const Degrees &angle) {
    stage()->actor(group_actor_id_)->rotate_y(angle);

}

void SpriteGrid::rotate_z(const Degrees &angle) {
    stage()->actor(group_actor_id_)->rotate_z(angle);
}

void SpriteGrid::look_at(const kglt::Vec3& target) {
    stage()->actor(group_actor_id_)->look_at(target);
}

void SpriteGrid::look_at(float x, float y, float z) {
    stage()->actor(group_actor_id_)->look_at(x, y, z);
}

Vec3 SpriteGrid::position() const {
    return stage()->actor(group_actor_id_)->position();
}

Vec2 SpriteGrid::position_2d() const {
    return stage()->actor(group_actor_id_)->position_2d();
}

Quaternion SpriteGrid::rotation() const {
    return stage()->actor(group_actor_id_)->rotation();
}

}
}
