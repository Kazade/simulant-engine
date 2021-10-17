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

#include "obj_loader.h"

#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "../shortcuts.h"
#include "../vfs.h"
#include "../utils/string.h"
#include "../application.h"
#include "../window.h"

namespace smlt {
namespace loaders {

struct LoadInfo {
    Mesh* target_mesh = nullptr;
    AssetManager* assets = nullptr;
    Material* current_material = nullptr;
    VertexData* vdata = nullptr;
    VertexSpecification vspec;
    CullMode cull_mode = CULL_MODE_BACK_FACE;

    std::istream* stream;
    Path folder;

    uint32_t line_offset = 0;
    uint32_t arg_offset = 0;
};

typedef std::function<bool (LoadInfo*, std::string, std::string)> CommandHandler;

typedef std::map<std::string, CommandHandler> CommandList;

static void run_parser(LoadInfo& info, const CommandList& commands) {
    auto& data_ = info.stream;

    std::string command;

    while(!data_->eof()) {
        auto c = data_->get();
        if(c == ' ' && !strip(command).empty()) {
            command = strip(command);

            /* Skip whitespace between the command
             * and the arguments */
            while(data_->peek() == ' ' || data_->peek() == '\t') {
                c = data_->get();
            }

            /* Store the offset to the first character
             * in the arglist */
            info.arg_offset = data_->tellg();

            std::string args;
            while(data_->good() && c != '\n') {
                c = data_->get();
                args += c;
            }

            args = strip(args);

            if(!commands.at(command)(&info, command, args)) {
                S_ERROR("Error passing command '{0}' with args: {1}", command, args);
                return;
            }

            command.clear();

            /* Set the line offset for the next line */
            info.line_offset = data_->tellg();
            info.line_offset++;

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

    if(info->target_mesh->has_submesh(mat_name)) {
        info->current_material = info->target_mesh->find_submesh(mat_name)->material().get();
    } else {
        auto sm = info->target_mesh->new_submesh(mat_name);
        info->current_material = sm->material().get();
        info->current_material->set_name(args);
        info->current_material->set_cull_mode(info->cull_mode);
    }

    return true;
}

static bool map_Kd(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ?
        info->current_material :
        info->target_mesh->find_submesh("__default__")->material().get();

    auto tex = info->assets->new_texture_from_file(args);
    mat->set_diffuse_map(tex);
    return true;
}

static bool Ka(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ?
        info->current_material :
        info->target_mesh->find_submesh("__default__")->material().get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_ambient(smlt::Colour(r, g, b, 1.0f));

    return true;
}

static bool Kd(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ?
        info->current_material :
        info->target_mesh->find_submesh("__default__")->material().get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_diffuse(smlt::Colour(r, g, b, 1.0f));

    return true;
}

static bool Ks(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ?
        info->current_material :
        info->target_mesh->find_submesh("__default__")->material().get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_specular(smlt::Colour(r, g, b, 1.0f));
    return true;
}

static bool Ke(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ?
        info->current_material :
        info->target_mesh->find_submesh("__default__")->material().get();

    auto parts = split(args);

    float r = smlt::stof(parts[0]);
    float g = smlt::stof(parts[1]);
    float b = smlt::stof(parts[2]);

    mat->set_emission(smlt::Colour(r, g, b, 1.0f));
    return true;
}

static bool Ns(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ?
        info->current_material :
        info->target_mesh->find_submesh("__default__")->material().get();

    float s = smlt::stof(args);

    mat->set_shininess(128.0f * (0.001f * s));
    return true;
}

static bool d(LoadInfo* info, std::string, std::string args) {
    Material* mat = (info->current_material) ?
        info->current_material :
        info->target_mesh->find_submesh("__default__")->material().get();

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

    auto& vfs = get_app()->window->vfs;

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

    auto sm = info->target_mesh->find_submesh(mat_name);

    if(sm) {
        info->current_material = sm->material().get();
        return true;
    }

    S_ERROR("Couldn't find submesh for material: {0}", mat_name);
    return false;
}

/*
 * Loading .obj files, while keeping memory usage low is very
 * tricky. Face corners are composed of separate positions, texcoords,
 * and normals. Combined those create the final vertex.
 *
 * The problem is, how do you generate those vertices without loading
 * all the data up front, and then duplicating it?
 *
 * The approach here is to store file offsets to the data, rather than the
 * actual data. This means that you save 2/3rds the memory usage but
 * incur a perf cost (moving around the file, and parsing the lines)
 */

static std::vector<uint32_t>* VERTEX_OFFSETS = nullptr;
static std::vector<uint32_t>* TEXCOORD_OFFSETS = nullptr;
static std::vector<uint32_t>* NORMAL_OFFSETS = nullptr;

static bool load_vertex(LoadInfo* info, std::string, std::string) {
    VERTEX_OFFSETS->push_back(info->arg_offset);
    return true;
}

static bool load_texcoord(LoadInfo* info, std::string, std::string) {
    if(!info->vspec.has_texcoord0()) {
        return true;
    }

    TEXCOORD_OFFSETS->push_back(info->arg_offset);
    return true;
}

static bool load_normal(LoadInfo* info, std::string, std::string) {
    if(!info->vspec.has_normals()) {
        return true;
    }

    NORMAL_OFFSETS->push_back(info->arg_offset);
    return true;
}

static uint8_t parse_floats(std::istream* stream, float* out, uint8_t count) {
    uint8_t current = 0;

    std::string args;
    auto c = stream->get();
    while(c != '\n') {
        if(c == ' ' || c == '\t') {
            out[current++] = smlt::stof(args);
            if(current == count) {
                return true;
            }

            args.clear();
        } else {
            args += c;
        }

        c = stream->get();
    }

    assert(!args.empty());
    out[current++] = smlt::stof(args);

    return current;
}

static bool load_face(LoadInfo* info, std::string, std::string args) {
    float xyzrgb[6] = {0, 0, 0, 1, 1, 1};

    smlt::SubMeshPtr submesh;
    if(info->current_material) {
        submesh = info->target_mesh->find_submesh(info->current_material->name());
    } else {
        if(info->target_mesh->has_submesh("__default__")) {
            submesh = info->target_mesh->find_submesh("__default__");
        } else {
            submesh = info->target_mesh->new_submesh("__default__");
        }
    }

    auto corners = split(args);

    auto stream_pos = info->stream->tellg();

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

        if(vindex == -1) {
            return false;
        } else {
            info->stream->seekg(VERTEX_OFFSETS->at(vindex - 1));
            parse_floats(info->stream, xyzrgb, 6);
            info->vdata->position(xyzrgb[0], xyzrgb[1], xyzrgb[2]);
        }

        if(tindex != -1 && info->vspec.has_texcoord0()) {
            info->stream->seekg(TEXCOORD_OFFSETS->at(tindex - 1));
            parse_floats(info->stream, xyzrgb, 2);
            info->vdata->tex_coord0(xyzrgb[0], xyzrgb[1]);
        }

        if(nindex != -1 && info->vspec.has_normals()) {
            info->stream->seekg(NORMAL_OFFSETS->at(nindex - 1));
            parse_floats(info->stream, xyzrgb, 3);
            info->vdata->normal(xyzrgb[0], xyzrgb[1], xyzrgb[2]);
        }

        if(info->vspec.has_diffuse()) {
            info->vdata->diffuse(smlt::Colour(xyzrgb[3], xyzrgb[4], xyzrgb[5], 1.0f));
        }

        info->vdata->move_next();
        submesh->index_data->index(info->vdata->count() - 1);
    }

    // Restore position
    info->stream->seekg(stream_pos);
    return true;
}


void OBJLoader::into(Loadable &resource, const LoaderOptions &options) {
    std::vector<uint32_t> _vertex_offsets, _texcoord_offsets, _normal_offsets;
    VERTEX_OFFSETS = &_vertex_offsets;
    TEXCOORD_OFFSETS = &_texcoord_offsets;
    NORMAL_OFFSETS = &_normal_offsets;

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

    info.folder = kfs::path::dir_name(filename_.str());

    run_parser(info, commands);

    vdata->done();
    for(auto& sm: mesh->each_submesh()) {
        sm->index_data->done();
    }
}

}
}
