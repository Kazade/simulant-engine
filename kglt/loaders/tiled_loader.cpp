#include "tiled_loader.h"
#include "../mesh.h"
#include "../types.h"
#include "../extra/tiled/TmxParser/Tmx.h"
#include "../resource_manager.h"

namespace kglt {
namespace loaders {

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

void TiledLoader::into(Loadable &resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);

    if(!mesh) {
        throw LogicError("Tried to load a TMX file into something that wasn't a mesh");
    }

    Tmx::Map map;

    map.ParseFile(this->filename_.encode());

    unicode layer_name = options.at("layer");

    auto layers = map.GetLayers();
    auto it = std::find_if(layers.begin(), layers.end(), [=](Tmx::Layer* layer) { return layer->GetName() == layer_name.encode(); });

    if(it == layers.end()) {
        throw RuntimeError(_u("Unable to find the layer with name: {0}").format(layer_name).encode());
    }

    Tmx::Layer* layer = (*it);

    unicode parent_dir = os::path::abs_path(os::path::dir_name(filename_));

    std::map<int32_t, TilesetInfo> tileset_info;

    std::map<int32_t, MaterialID> tileset_materials;


    //Load all of the tilesets from the map
    for(int32_t i = 0; i < map.GetNumTilesets(); ++i) {
        const Tmx::Tileset* tileset = map.GetTileset(i);
        const Tmx::Image* image = tileset->GetImage();
        unicode rel_path = image->GetSource();

        unicode final_path = os::path::join(parent_dir, rel_path);
        L_DEBUG(_u("Loading tileset from: {0}").format(final_path));

        TextureID tid = mesh->resource_manager().new_texture_from_file(
            final_path,
            TEXTURE_OPTION_CLAMP_TO_EDGE | TEXTURE_OPTION_DISABLE_MIPMAPS | TEXTURE_OPTION_NEAREST_FILTER
        );

        tileset_materials[i] = mesh->resource_manager().new_material_from_texture(tid);

        TilesetInfo& info = tileset_info[i];
        info.margin = tileset->GetMargin();
        info.spacing = tileset->GetSpacing();
        info.tile_height = tileset->GetTileHeight();
        info.tile_width = tileset->GetTileWidth();
        info.total_height = image->GetHeight();
        info.total_width = image->GetWidth();
    }

    /*
      Now go through the layer and build up a tile submesh for each grid square. Originally I chunked these
      tiles into groups for nicer culling but this is the wrong place for that. If rendering a lot of submeshes
      is inefficient then that needs to be tackled in the partitioner.
    */

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

            //Create the submesh as a rectangle, the offset determines the location on the map
            auto sidx = mesh->new_submesh_as_rectangle(tileset_materials.at(tileset_index), 1.0, 1.0, Vec3(-0.5, -0.5, 0));

            //Set texture coordinates appropriately for the tileset
            float tx0 = x0 / tileset.total_width + (0.5 / tileset.total_width);
            float ty0 = y0 / tileset.total_height - (0.5 / tileset.total_height);
            float tx1 = x1 / tileset.total_width - (0.5 / tileset.total_width);
            float ty1 = y1 / tileset.total_height + (0.5 / tileset.total_height);

            mesh->submesh(sidx).vertex_data().move_to(0);
            mesh->submesh(sidx).vertex_data().tex_coord0(kglt::Vec2(tx0, ty1));

            mesh->submesh(sidx).vertex_data().move_next();
            mesh->submesh(sidx).vertex_data().tex_coord0(kglt::Vec2(tx1, ty1));

            mesh->submesh(sidx).vertex_data().move_next();
            mesh->submesh(sidx).vertex_data().tex_coord0(kglt::Vec2(tx1, ty0));

            mesh->submesh(sidx).vertex_data().move_next();
            mesh->submesh(sidx).vertex_data().tex_coord0(kglt::Vec2(tx0, ty0));

            mesh->submesh(sidx).vertex_data().done();
        }
    }
}

}
}
