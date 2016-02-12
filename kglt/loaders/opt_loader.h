#ifndef OPT_LOADER_H
#define OPT_LOADER_H

#include <map>
#include <vector>
#include "../types.h"
#include "../loader.h"

namespace kglt {
namespace loaders {

typedef int32_t Offset;

class OPTLoader : public Loader {
public:
    OPTLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options=LoaderOptions());

private:
    void read_block(std::istream& file, Offset offset);
    void process_vertex_block(std::istream& file);
    void process_texcoord_block(std::istream& file);
    void process_normal_block(std::istream& file);
    void process_face_block(std::istream& file);
    void process_reused_texture_block(std::istream& file);
    void process_lod_block(std::istream& file);
    void process_embedded_texture_block(std::istream& file);

    Offset global_offset;

    /* Temp Data storage */
    std::vector<Vec3> vertices;
    std::vector<Vec2> texture_vertices;
    std::vector<Vec3> vertex_normals;

    struct Texture {
        std::string name;
        int32_t width;
        int32_t height;
        int32_t bytes_per_pixel;
        std::vector<uint8_t> data;
    };
    std::vector<Texture> textures;
    std::string current_texture;

    std::map<std::string, SubMeshID> texture_submesh;
    std::map<std::string, TextureID> texture_name_to_id;

    struct Triangle {
        Vec3 positions[3];
        Vec2 tex_coords[3];
        Vec3 normals[3];

        Vec3 face_normal;
        std::string texture_name;
    };
    std::vector<std::vector<Triangle> > triangles; //Triangles for each LOD
    uint8_t current_lod;
};

class OPTLoaderType : public LoaderType {
public:
    unicode name() { return "opt"; }
    bool supports(const unicode& filename) const {
        //FIXME: check magic
        return filename.lower().contains(".opt");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new OPTLoader(filename, data));
    }
};


}
}

#endif // OPT_LOADER_H
