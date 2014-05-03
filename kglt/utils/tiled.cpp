
#include "tiled.h"
#include "../mesh.h"

namespace kglt {

Vec2 pixel_to_world(const MeshPtr& mesh, const Vec2& pixel_position) {
    //FIXME: If the mesh has been scaled this won't work...

    int map_tile_width = mesh->get<int>("TILED_MAP_TILE_WIDTH");
    int map_tile_height = mesh->get<int>("TILED_MAP_TILE_HEIGHT");

    return Vec2(
        (pixel_position.x / float(map_tile_width)),
        (pixel_position.y / float(map_tile_height))
    );
}

}
