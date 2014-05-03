
#include "tiled.h"
#include "../mesh.h"

namespace kglt {
namespace tiled {

Vec2 pixel_to_world(const MeshPtr& mesh, const Vec2& pixel_position) {
    //FIXME: If the mesh has been scaled this won't work...

    int map_tile_width = mesh->get<int>("TILED_MAP_TILE_WIDTH");
    int map_tile_height = mesh->get<int>("TILED_MAP_TILE_HEIGHT");
    float render_size = mesh->get<float>("TILED_TILE_RENDER_SIZE");

    return Vec2(
        ((pixel_position.x / float(map_tile_width)) * render_size),
        ((pixel_position.y / float(map_tile_height)) * render_size)
    );
}

}
}
