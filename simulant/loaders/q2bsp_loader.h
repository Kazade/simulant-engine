/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef Q2BSP_LOADER_H_INCLUDED
#define Q2BSP_LOADER_H_INCLUDED

#include <map>

#include "../loader.h"

namespace smlt {

typedef std::map<std::string, std::string> Q2Entity;
typedef std::vector<Q2Entity> Q2EntityList;

namespace loaders {

namespace Q2 {

enum LumpType {
    ENTITIES = 0,
    PLANES,
    VERTICES,
    VISIBILITY,
    NODES,
    TEXTURE_INFO,
    FACES,
    LIGHTMAPS,
    LEAVES,
    LEAF_FACE_TABLE,
    LEAF_BRUSH_TABLE,
    EDGES,
    FACE_EDGE_TABLE,
    MODELS,
    BRUSHES,
    BRUSH_SIDES,
    POP,
    AREAS,
    AREA_PORTALS,
    MAX_LUMPS
};

typedef Vec3 Point3f;

struct Point3s {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct Edge {
    uint16_t a;
    uint16_t b;
};

struct Plane {
    Point3f normal;
    float distance;
    uint32_t type;
};

enum SurfaceFlag {
    SURFACE_FLAG_NONE = 0x0,
    SURFACE_FLAG_LIGHT = 0x1,
    SURFACE_FLAG_SLICK = 0x2,
    SURFACE_FLAG_SKY = 0x4,
    SURFACE_FLAG_WARP = 0x8,
    SURFACE_FLAG_TRANS_33 = 0x10,
    SURFACE_FLAG_TRANS_66 = 0x20,
    SURFACE_FLAG_FLOWING = 0x40,
    SURFACE_FLAG_NO_DRAW = 0x80
};

struct TextureInfo {
    Point3f u_axis;
    float u_offset;
    Point3f v_axis;
    float v_offset;

    SurfaceFlag flags;
    uint32_t value;

    char texture_name[32];

    uint32_t next_tex_info;
};

struct Face {
    uint16_t plane;             // index of the plane the face is parallel to
    uint16_t plane_side;        // set if the normal is parallel to the plane normal
    uint32_t first_edge;        // index of the first edge (in the face edge array)
    uint16_t num_edges;         // number of consecutive edges (in the face edge array)
    uint16_t texture_info;      // index of the texture info structure
    uint8_t lightmap_syles[4]; // styles (bit flags) for the lightmaps
    uint32_t lightmap_offset;   // offset of the lightmap (in bytes) in the lightmap lump
};

struct Lump {
    uint32_t offset;
    uint32_t length;
};

struct Header {
    uint8_t magic[4];
    uint32_t version;

    Lump lumps[MAX_LUMPS];
};

struct TexDimension {
    uint32_t width;
    uint32_t height;

    TexDimension(uint32_t width, uint32_t height):
        width(width), height(height) {}
};

}

class Q2BSPLoader : public Loader {
public:
    Q2BSPLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options=LoaderOptions());

private:
    void generate_materials(
        AssetManager *manager,
        const std::vector<Q2::TextureInfo>& texture_infos,
        std::vector<MaterialID>& materials,
        std::vector<Q2::TexDimension>& dimensions,
        TexturePtr lightmap_texture
    );

};

class Q2BSPLoaderType : public LoaderType {
public:
    const char* name() { return "bsp_loader"; }
    bool supports(const Path& filename) const {
        //FIXME: check magic
        return filename.ext() == ".bsp";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const {
        return Loader::ptr(new Q2BSPLoader(filename, data));
    }
};


}
}


#endif // Q2BSP_LOADER_H_INCLUDED
