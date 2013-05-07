#include "obj_loader.h"

#include "../mesh.h"
#include "../scene.h"
#include "../resource_manager.h"
#include "../kazbase/unicode.h"
#include "../kazbase/string.h"
#include "../kazbase/file_utils.h"

namespace kglt {
namespace loaders {

void parse_face(const unicode& input, int32_t& vertex_index, int32_t& tex_index, int32_t& normal_index) {
    /*
     *  Parses the following
     *  1//2
     *  1
     *  1/1
     *  1/1/1
     *  and outputs to the passed references. The index will equal -1 if it wasn't in the input
     */

    std::vector<unicode> parts = input.split("/");
    int32_t slash_count = input.count("/");

    if(slash_count == 0 || input.ends_with("//")) {
        tex_index = -1;
        normal_index = -1;
        vertex_index = parts[0].to_int();
    } else if(slash_count == 1) {
        normal_index = -1;
        vertex_index = parts[0].to_int();
        tex_index = parts[1].to_int();
    } else if(slash_count == 2) {
        if(input.contains("//")) {
            tex_index = -1;
        } else {
            tex_index = parts[1].to_int();
        }

        vertex_index = parts[0].to_int();
        normal_index = parts[parts.size() - 1].to_int();
    }

    //Handle the 1-based indexing
    if(vertex_index > -1) {
        vertex_index -= 1;
    }

    if(tex_index > -1) {
        tex_index -= 1;
    }

    if(normal_index > -1) {
        normal_index -= 1;
    }
}

void OBJLoader::into(Loadable &resource, const LoaderOptions &options) {
    Mesh* mesh = loadable_to<Mesh>(resource);

    //Create a submesh with the default material
    SubMeshIndex smi = mesh->new_submesh(
        mesh->resource_manager().scene().default_material_id()
    );

    SubMesh& sm = mesh->submesh(smi);

    //Read all the file content
    std::string file_content = file_utils::read(filename_.encode());

    //Split on newlines
    std::vector<std::string> lines = str::split(file_content, "\n");

    std::vector<Vec3> vertices;
    std::vector<Vec2> tex_coords;
    std::vector<Vec3> normals;

    bool first_vertex = true;
    for(std::string tmp: lines) {
        unicode line(tmp); //Unicode has nicer methods

        //Ignore comments
        if(line.strip().starts_with("#")) {
            continue;
        }

        std::vector<unicode> parts = line.strip().split(" \t"); //Split on whitespace
        if(parts[0] == "v") {
            if(parts.size() != 4) {
                throw IOError(_u("Found {0} components for vertex, expected 3").format(parts.size()));
            }

            float x = parts[1].to_float();
            float y = parts[2].to_float();
            float z = parts[3].to_float();

            vertices.push_back(Vec3(x, y, z));
        } else if(parts[0] == "vt") {
            if(parts.size() < 2) {
                throw IOError(_u("Found {0} components for texture coordinate, expected 2").format(parts.size()));
            } else if(parts.size() > 2) {
                L_WARN("Ignoring extra components on texture coordinate");
            }

            float x = parts[1].to_float();
            float y = parts[2].to_float();

            tex_coords.push_back(Vec2(x, y));

        } else if(parts[0] == "vn") {
            if(parts.size() != 4) {
                throw IOError(_u("Found {0} components for vertex, expected 3").format(parts.size()));
            }

            float x = parts[1].to_float();
            float y = parts[2].to_float();
            float z = parts[3].to_float();

            Vec3 n(x, y, z);
            kmVec3Normalize(&n, &n);
            normals.push_back(n);
        } else if(parts[0] == "f") {
            //Faces are a pain in the arse to parse
            parts = std::vector<unicode>(parts.begin() + 1, parts.end()); //Strip off the first bit

            int32_t first_v, first_vt, first_vn;
            parse_face(parts[0], first_v, first_vt, first_vn);

            int32_t first_index = sm.vertex_data().count(); //Store the index of this vertex

            sm.vertex_data().position(vertices[first_v]); //Read the position
            sm.vertex_data().tex_coord0(tex_coords[first_vt]);
            sm.vertex_data().tex_coord1(tex_coords[first_vt]);
            sm.vertex_data().normal(normals[first_vn]);
            sm.vertex_data().diffuse(kglt::Colour::white);
            sm.vertex_data().move_next();

            parts = std::vector<unicode>(parts.begin() + 1, parts.end()); //Strip off the first bit

            for(uint32_t i = 1; i < parts.size(); i++) {
                int32_t v1, vt1, vn1;
                int32_t v2, vt2, vn2;

                parse_face(parts[i-1], v1, vt1, vn1);
                parse_face(parts[i], v2, vt2, vn2);

                sm.vertex_data().position(vertices[v1]); //Read the position
                sm.vertex_data().tex_coord0(tex_coords[vt1]);
                sm.vertex_data().tex_coord1(tex_coords[vt1]);
                sm.vertex_data().normal(normals[vn1]);
                sm.vertex_data().diffuse(kglt::Colour::white);
                sm.vertex_data().move_next();

                sm.vertex_data().position(vertices[v2]); //Read the position
                sm.vertex_data().tex_coord0(tex_coords[vt2]);
                sm.vertex_data().tex_coord1(tex_coords[vt2]);
                sm.vertex_data().normal(normals[vn2]);
                sm.vertex_data().diffuse(kglt::Colour::white);
                sm.vertex_data().move_next();

                //Add the triangle
                sm.index_data().index(first_index);
                sm.index_data().index(sm.vertex_data().count() - 2);
                sm.index_data().index(sm.vertex_data().count() - 1);
            }
        }
    }

    sm.vertex_data().done();
    sm.index_data().done();
}

}
}
