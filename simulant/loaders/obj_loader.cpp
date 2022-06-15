//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//
#include <string>
#include <map>

#include "obj_loader.h"

#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "../shortcuts.h"
#include "../vfs.h"
#include "../utils/string.h"
#include "../application.h"
#include "../window.h"

#include "../utils/packed_types.h"

namespace smlt {
namespace loaders {

enum VertexBatchType {
    VERTEX_BATCH_TYPE_TRIANGLES,
    VERTEX_BATCH_TYPE_FANS
};

struct VertexDataBatch {
    std::string material_name;
    VertexBatchType type;
    std::shared_ptr<VertexData> data;
    std::vector<int> ranges;
};

struct LoadInfo {
    Mesh* target_mesh = nullptr;
    AssetManager* assets = nullptr;

    MaterialPtr default_material = nullptr;
    std::map<std::string, MaterialPtr> materials;

    Material* current_material = nullptr;

    VertexData* vdata = nullptr;
    VertexSpecification vspec;

    CullMode cull_mode = CULL_MODE_BACK_FACE;
    std::string overridden_tex_format = "";

    std::istream* stream;
    Path folder;

    std::list<VertexDataBatch> batches;
};

typedef std::function<bool (LoadInfo*, std::string, std::string)> CommandHandler;

typedef std::map<std::string, CommandHandler> CommandList;

static void run_parser(LoadInfo& info, const CommandList& commands) {
    auto& data_ = info.stream;

    std::string command;

    // Only used for debug logging
    std::string last_command;

    while(!data_->eof()) {
        auto c = data_->get();
        if((c == ' ' || c == '\n')  && !strip(command).empty()) {
            command = strip(command);

            if(command != last_command) {
                S_DEBUG("OBJ: Found new block: {0}", command);
                last_command = command;
            }

            /* Skip whitespace between the command
             * and the arguments */
            while(data_->peek() == ' ' || data_->peek() == '\t') {
                c = data_->get();
            }

            std::string args;
            while(data_->good() && c != '\n') {
                c = data_->get();
                args += c;
            }

            args = strip(args);

            if(!commands.count(command)) {
                S_WARN("Unhandled OBJ command: {0}", command);
                command.clear();
                continue;
            }

            if(!commands.at(command)(&info, command, args)) {
                S_ERROR("Error passing command '{0}' with args: {1}", command, args);
                return;
            }

            command.clear();
        } else if(c != ' ') {
            command += c;
        }
    }
}

static bool null(LoadInfo*, std::string, std::string) {
    return true;
}

static bool newmtl(LoadInfo* info, std::string, std::string args) {
    auto mat_name = strip(args);

    if(info->materials.count(mat_name)) {
        info->current_material = info->materials.at(mat_name).get();
    } else {
        auto new_mat = info->assets->clone_default_material();
        new_mat->set_name(args);
        new_mat->set_cull_mode(info->cull_mode);

        info->materials[mat_name] = new_mat;
        info->current_material = new_mat.get();
    }

    return true;
}

static bool map_Kd(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ? info->current_material : info->default_material.get();

    /* Handle replacing the texture extension if that was desired */
    if(!info->overridden_tex_format.empty()) {
        auto ext = info->overridden_tex_format;
        if(ext[0] != '.') {
            ext = "." + ext;
        }

        args = kfs::path::split_ext(args).first + ext;
    }

    auto tex = info->assets->new_texture_from_file(args);

    /* Force upload to VRAM and free the RAM */
    tex->flush();

    mat->set_diffuse_map(tex);
    return true;
}

static bool Ka(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ? info->current_material : info->default_material.get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_ambient(smlt::Colour(r, g, b, 1.0f));

    return true;
}

static bool Kd(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ? info->current_material : info->default_material.get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_diffuse(smlt::Colour(r, g, b, 1.0f));

    return true;
}

static bool Ks(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ? info->current_material : info->default_material.get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_specular(smlt::Colour(r, g, b, 1.0f));
    return true;
}

static bool Ke(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ? info->current_material : info->default_material.get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_emission(smlt::Colour(r, g, b, 1.0f));
    return true;
}

static bool Ns(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ? info->current_material : info->default_material.get();

    float s = smlt::stof(args);

    mat->set_shininess(128.0f * (0.001f * s));
    return true;
}

static bool d(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ? info->current_material : info->default_material.get();

    float d = smlt::stof(args);
    if(almost_equal(d, 1.0f)) {
        mat->set_blend_func(smlt::BLEND_NONE);
    } else {
        mat->set_blend_func(smlt::BLEND_ALPHA);
    }

    auto c = mat->ambient();
    c.a = d;
    mat->set_ambient(c);

    c = mat->diffuse();
    c.a = d;
    mat->set_diffuse(c);

    c = mat->specular();
    c.a = d;
    mat->set_specular(c);

    c = mat->emission();
    c.a = d;
    mat->set_emission(c);
    return true;
}

static bool load_material_lib(LoadInfo* info, std::string, std::string args) {
    const std::map<std::string, CommandHandler> commands = {
        {"newmtl", newmtl},
        {"Ka", Ka},
        {"Kd", Kd},
        {"Ks", Ks},
        {"Ke", Ke},
        {"Ns", Ns},
        {"Ni", null},
        {"d", d},
        {"illum", null},
        {"map_Kd", map_Kd},
        {"#", null},
    };

    auto stash = info->stream;

    auto& vfs = get_app()->vfs;

    auto added = vfs->add_search_path(info->folder);

    auto mtl_stream = vfs->open_file(args);

    info->stream = mtl_stream.get();

    run_parser(*info, commands);

    if(added) {
        vfs->remove_search_path(info->folder);
    }

    info->stream = stash;

    return true;
}

static bool apply_material(LoadInfo* info, std::string, std::string args) {
    auto mat_name = strip(args);

    if(info->materials.count(mat_name)) {
        info->current_material = info->materials[mat_name].get();
        return true;
    }

    S_ERROR("Couldn't find submesh for material: {0}", mat_name);
    return false;
}

static std::vector<HalfVec3>* VERTICES = nullptr;
static std::vector<HalfVec3>* COLOURS = nullptr;
static std::vector<HalfVec2>* TEXCOORDS = nullptr;
static std::vector<HalfVec3>* NORMALS = nullptr;

static uint8_t parse_floats(std::string stream, float* out, uint8_t count) {
    uint8_t current = 0;

    char buffer[32];
    uint8_t buffer_c = 0;

    for(auto& c: stream) {
        if(c == ' ' || c == '\t') {
            assert(buffer_c < 32 - 1);
            buffer[buffer_c++] = '\0';
            out[current++] = atof(buffer);
            if(current == count) {
                return true;
            }

            buffer_c = 0;
        } else {
            assert(buffer_c < 32 - 1);
            buffer[buffer_c++] = c;
        }
    }

    assert(buffer_c > 0);
    assert(buffer_c < 32 - 1);
    buffer[buffer_c++] = '\0';
    out[current++] = atof(buffer);

    return current;
}

static bool load_vertex(LoadInfo*, std::string, std::string args) {
    float xyzrgb[6] = {0, 0, 0, 1, 1, 1};

    parse_floats(args, xyzrgb, 6);
    VERTICES->push_back(HalfVec3(xyzrgb[0], xyzrgb[1], xyzrgb[2]));
    COLOURS->push_back(HalfVec3(xyzrgb[3], xyzrgb[4], xyzrgb[5]));
    return true;
}

static bool load_texcoord(LoadInfo* info, std::string, std::string args) {
    if(!info->vspec.has_texcoord0()) {
        return true;
    }

    float uv[2] = {0, 0};

    parse_floats(args, uv, 2);
    TEXCOORDS->push_back(HalfVec2(uv[0], uv[1]));
    return true;
}

static bool load_normal(LoadInfo* info, std::string, std::string args) {
    if(!info->vspec.has_normals()) {
        return true;
    }

    float nxyz[3] = {0, 0, 0};

    parse_floats(args, nxyz, 3);
    NORMALS->push_back(HalfVec3(nxyz[0], nxyz[1], nxyz[2]));
    return true;
}


static bool load_face(LoadInfo* info, std::string, std::string args) {
    /* To render things as efficiently as possible, we need two submeshes per material:
     *
     * 1. For triangles, rendered as a single "draw arrays" call
     * 2. For triangle-fans, rendered as a series of "draw arrays" calls
     *
     * Because these things all need the same vertex data, we need to essentially
     * "cache" the data into separate arrays and the inject them into the
     * final data as:
     *
     * [mat0tris, mat0fans, mat1tris, mat1fans, ...]
     *
     */

    auto find_batch = [info](const std::string& material_name, VertexBatchType type) -> VertexDataBatch* {
        for(auto& batch: info->batches) {
            if(batch.material_name == material_name && batch.type == type) {
                return &batch;
            }
        }

        VertexDataBatch new_batch;
        new_batch.material_name = material_name;
        new_batch.type = type;
        new_batch.data = std::make_shared<VertexData>(info->vspec);
        info->batches.push_back(new_batch);
        return &info->batches.back();
    };

    VertexDataBatch* batches[2];

    batches[0] =
        (info->current_material) ?
            find_batch(info->current_material->name(), VERTEX_BATCH_TYPE_TRIANGLES) :
            find_batch("__default__", VERTEX_BATCH_TYPE_TRIANGLES);

    batches[1] =
        (info->current_material) ?
            find_batch(info->current_material->name(), VERTEX_BATCH_TYPE_FANS) :
            find_batch("__default__", VERTEX_BATCH_TYPE_FANS);

    assert(batches[0]);
    assert(batches[1]);

    auto corners = split(args);

    auto batch = (corners.size() == 3) ? batches[0] : batches[1];
    batch->ranges.push_back(corners.size());

    for(auto& corner: corners) {
        int32_t vindex = -1, tindex = -1, nindex = -1;
        auto parts = split(corner, "/");
        if(corner.find("//") != std::string::npos) {
            vindex = (parts[0].empty()) ? -1 : smlt::stoi(parts[0]);
            nindex = (parts[1].empty()) ? -1 : smlt::stoi(parts[1]);
        } else if(parts.size() == 2) {
            vindex = (parts[0].empty()) ? -1 : smlt::stoi(parts[0]);
            tindex = (parts[1].empty()) ? -1 : smlt::stoi(parts[1]);
        } else if(parts.size() == 1) {
            vindex = (parts[0].empty()) ? -1 : smlt::stoi(parts[0]);
        } else {
            assert(parts.size() == 3);
            vindex = (parts[0].empty()) ? -1 : smlt::stoi(parts[0]);
            tindex = (parts[1].empty()) ? -1 : smlt::stoi(parts[1]);
            nindex = (parts[2].empty()) ? -1 : smlt::stoi(parts[2]);
        }

        smlt::Colour diffuse = smlt::Colour::WHITE;
        if(vindex == -1) {
            return false;
        } else if((vindex - 1) < (int) VERTICES->size()) {
            Vec3 p = VERTICES->at(vindex - 1);
            Vec3 c = COLOURS->at(vindex - 1);

            diffuse.r = c.x;
            diffuse.g = c.y;
            diffuse.b = c.z;

            batch->data->position(p);
        } else {
            S_WARN("Invalid vertex index {0} while loading model", vindex - 1);
            batch->data->position(smlt::Vec3());
        }

        if(tindex != -1 && info->vspec.has_texcoord0()) {
            Vec2 t = TEXCOORDS->at(tindex - 1);
            batch->data->tex_coord0(t);
        }

        if(nindex != -1 && info->vspec.has_normals()) {
            Vec3 n = NORMALS->at(nindex - 1);
            batch->data->normal(n);
        }

        if(info->vspec.has_diffuse()) {
            batch->data->diffuse(diffuse);
        }

        batch->data->move_next();
    }       
    return true;
}


void OBJLoader::into(Loadable &resource, const LoaderOptions &options) {
    std::vector<HalfVec3> _vertices, _colours, _normals;
    std::vector<HalfVec2> _texcoords;

    VERTICES = &_vertices;
    TEXCOORDS = &_texcoords;
    NORMALS = &_normals;
    COLOURS = &_colours;

    Mesh* mesh = loadable_to<Mesh>(resource);

    S_DEBUG("Loading mesh from {0}", filename_);

    MeshLoadOptions mesh_opts;
    auto it = options.find(MESH_LOAD_OPTIONS_KEY);

    if(it != options.end()) {
        mesh_opts = smlt::any_cast<MeshLoadOptions>(it->second);
    }

    S_DEBUG("Got MeshOptions");
    S_DEBUG("About to read the obj model");

    auto spec = mesh->vertex_data->vertex_specification();
    mesh->reset(spec);  /* Clear the mesh */

    auto vdata = mesh->vertex_data.get();

    const std::map<std::string, CommandHandler> commands = {
        {"mtllib", load_material_lib},
        {"usemtl", apply_material},
        {"v", load_vertex},
        {"vt", load_texcoord},
        {"vn", load_normal},
        {"#", null},
        {"g", null},
        {"f", load_face},
        {"o", null},
        {"s", null}
    };

    LoadInfo info;
    info.target_mesh = mesh;
    info.vdata = vdata;
    info.vspec = spec;
    info.assets = &mesh->asset_manager();
    info.stream = data_.get();
    info.cull_mode = mesh_opts.cull_mode;
    info.overridden_tex_format = mesh_opts.override_texture_extension;
    info.default_material = mesh->asset_manager().clone_default_material();
    info.folder = kfs::path::dir_name(filename_.str());

    run_parser(info, commands);

    /* OK now transfer the vertex data from the batches! */
    for(auto& batch: info.batches) {
        if(batch.type == VERTEX_BATCH_TYPE_TRIANGLES) {
            auto sm = mesh->find_submesh(batch.material_name);
            if(!sm) {
                if(batch.material_name == "__default__") {
                    sm = mesh->new_submesh(batch.material_name, info.default_material);
                } else {
                    sm = mesh->new_submesh(batch.material_name, info.materials.at(batch.material_name));
                }
            }

            sm->add_vertex_range(mesh->vertex_data->count(), batch.data->count());
            mesh->vertex_data->extend(*batch.data);
            batch.data.reset();
        } else {
            /* Same thing, but with a triangle fan arrangement */
            auto sm_name = batch.material_name + "_fan";
            auto sm = mesh->find_submesh(sm_name);
            if(!sm) {
                if(batch.material_name == "__default__") {
                    sm = mesh->new_submesh(sm_name, info.default_material, MESH_ARRANGEMENT_TRIANGLE_FAN);
                } else {
                    sm = mesh->new_submesh(sm_name, info.materials.at(batch.material_name), MESH_ARRANGEMENT_TRIANGLE_FAN);
                }
            }
            auto start = mesh->vertex_data->count();
            for(auto range: batch.ranges) {
                sm->add_vertex_range(start, range);
                start += range;
            }
            mesh->vertex_data->extend(*batch.data);
            batch.data.reset();
        }
    }

    vdata->done();

    /* Final nicety - search for diffuse/specular/bump maps in the current directory */
    std::string extensions [] = {
        ".jpg",
        ".png",
        ".tga"
    };

    for(auto& ext: extensions) {
        auto path = this->filename_.replace_ext(ext);
        if(kfs::path::exists(path.str().c_str())) {
            auto tex = mesh->asset_manager().new_texture_from_file(path);
            info.default_material->set_diffuse_map(tex);
        } else {
            path = kfs::path::split_ext(filename_.str()).first + "_color" + ext;
            if(kfs::path::exists(path.str().c_str())) {
                auto tex = mesh->asset_manager().new_texture_from_file(path);
                info.default_material->set_diffuse_map(tex);
            }
        }
    }

}

}
}
