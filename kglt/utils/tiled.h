#ifndef TILED_H
#define TILED_H

#include "../types.h"

namespace kglt {
namespace tiled {
    //Given a mesh and a pixel coordinate, returns the world coordinate using the tile width/height
    //stored when loading from a TMX file
    Vec2 pixel_to_world(const MeshPtr& mesh, const Vec2& pixel_position);

    float get_tile_render_size(const MeshPtr& mesh);
}
}

#endif // TILED_H
