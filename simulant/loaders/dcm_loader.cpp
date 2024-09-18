#include "dcm_loader.h"
#include "../application.h"
#include "../asset_manager.h"
#include "../meshes/mesh.h"
#include "../utils/limited_string.h"
#include "../vertex_data.h"
#include "../vfs.h"
#include "dcm.h"
#include <cstdint>

namespace smlt {
namespace loaders {

using namespace dcm;

VertexFormat determine_spec(const FileHeader& header) {
    /* FIXME:
     * - Support short UV format
     * - Support 3UB color format
     */

    VertexFormatBuilder builder;

    switch(header.pos_format) {
        case POSITION_FORMAT_2F:
            builder =
                builder.add(VERTEX_ATTR_NAME_POSITION,
                            VERTEX_ATTR_ARRANGEMENT_XY, VERTEX_ATTR_TYPE_FLOAT);
            break;
        case POSITION_FORMAT_3F:
            builder = builder.add(VERTEX_ATTR_NAME_POSITION,
                                  VERTEX_ATTR_ARRANGEMENT_XYZ,
                                  VERTEX_ATTR_TYPE_FLOAT);
            break;
        case POSITION_FORMAT_4F:
            builder = builder.add(VERTEX_ATTR_NAME_POSITION,
                                  VERTEX_ATTR_ARRANGEMENT_XYZW,
                                  VERTEX_ATTR_TYPE_FLOAT);
            break;
    }

    switch(header.tex0_format) {
        case TEX_COORD_FORMAT_2F:
            builder =
                builder.add(VERTEX_ATTR_NAME_TEXCOORD_0,
                            VERTEX_ATTR_ARRANGEMENT_XY, VERTEX_ATTR_TYPE_FLOAT);
            break;
    }

    switch(header.tex1_format) {
        case TEX_COORD_FORMAT_2F:
            builder =
                builder.add(VERTEX_ATTR_NAME_TEXCOORD_1,
                            VERTEX_ATTR_ARRANGEMENT_XY, VERTEX_ATTR_TYPE_FLOAT);
            break;
    }

    switch(header.color_format) {
        case COLOR_FORMAT_4UB:
            builder = builder.add(VERTEX_ATTR_NAME_COLOR,
                                  VERTEX_ATTR_ARRANGEMENT_RGBA,
                                  VERTEX_ATTR_TYPE_UNSIGNED_BYTE);
            break;
        case COLOR_FORMAT_4F:
            builder = builder.add(VERTEX_ATTR_NAME_COLOR,
                                  VERTEX_ATTR_ARRANGEMENT_RGBA,
                                  VERTEX_ATTR_TYPE_FLOAT);
            break;
        case COLOR_FORMAT_3F:
            builder =
                builder.add(VERTEX_ATTR_NAME_COLOR, VERTEX_ATTR_ARRANGEMENT_RGB,
                            VERTEX_ATTR_TYPE_FLOAT);
            break;
    }

    switch(header.normal_format) {
        case NORMAL_FORMAT_3F:
            builder = builder.add(VERTEX_ATTR_NAME_NORMAL,
                                  VERTEX_ATTR_ARRANGEMENT_XYZ,
                                  VERTEX_ATTR_TYPE_FLOAT);
            break;
    }

    return builder.build();
}

void DCMLoader::into(Loadable& resource, const LoaderOptions& options) {
    Mesh* mesh = loadable_to<Mesh>(resource);

    S_DEBUG("Loading mesh from {0}", filename_);

    MeshLoadOptions mesh_opts;
    auto it = options.find(MESH_LOAD_OPTIONS_KEY);

    if(it != options.end()) {
        mesh_opts = smlt::any_cast<MeshLoadOptions>(it->second);
    }

    FileHeader fheader;
    data_->read((char*) &fheader, sizeof(FileHeader));

    if(fheader.version != DCM_CURRENT_VERSION) {
        S_ERROR("Unsupported dcm version: {0}", fheader.version);
        return;
    }

    LimitedString<3> hid((const char*)fheader.id);

    if(hid != std::string("DCM")) {
        S_ERROR("Not a valid .dcm file: {0}", fheader.id);
        return;
    }

    auto spec = determine_spec(fheader);
    mesh->reset(spec);

    std::vector<::smlt::MaterialPtr> materials;

    /* Add the parent directory to the search path so we can load textures */
    auto added = smlt::get_app()->vfs->insert_search_path(0, filename_.parent());

    auto read_data_header = [=](DataHeader* out) {
        data_->read((char*) &out->flags, sizeof(out->flags));
        data_->read((char*) &out->local_id, sizeof(out->local_id));
        data_->read((char*) &out->path, sizeof(out->path));
    };

    for(int i = 0; i < fheader.material_count; ++i) {
        dcm::Material mat;
        read_data_header(&mat.data_header);

        data_->read((char*) mat.ambient, sizeof(mat.ambient));
        data_->read((char*) mat.diffuse, sizeof(mat.diffuse));
        data_->read((char*) mat.specular, sizeof(mat.specular));
        data_->read((char*) mat.emission, sizeof(mat.emission));
        data_->read((char*) &mat.shininess, sizeof(mat.shininess));

        data_->read((char*) mat.diffuse_map, sizeof(mat.diffuse_map));
        data_->read((char*) mat.light_map, sizeof(mat.light_map));
        data_->read((char*) mat.normal_map, sizeof(mat.normal_map));
        data_->read((char*) mat.specular_map, sizeof(mat.specular_map));

        smlt::MaterialPtr new_mat =
            mesh->asset_manager().clone_default_material();
        new_mat->set_pass_count(1);
        new_mat->set_lighting_enabled(true);
        new_mat->set_cull_mode(mesh_opts.cull_mode);
        new_mat->set_blend_func(mesh_opts.blending_enabled ? BLEND_ALPHA : BLEND_NONE);
        new_mat->set_diffuse(smlt::Color(mat.diffuse, 4));
        new_mat->set_ambient(smlt::Color(mat.ambient, 4));
        new_mat->set_specular(smlt::Color(mat.specular, 4));
        new_mat->set_emission(smlt::Color(mat.emission, 4));
        new_mat->set_shininess(mat.shininess * 128.0f);
        new_mat->set_name(
            std::string(mat.data_header.path, sizeof(mat.data_header.path))
                .c_str());

        int enabled_textures = 0;

        if(mat.diffuse_map[0]) {
            Path final = Path(std::string(mat.diffuse_map, 32).c_str());
            if(!mesh_opts.override_texture_extension.empty()) {
                final = final.replace_ext(mesh_opts.override_texture_extension);
            }

            auto tex = mesh->asset_manager().load_texture(final);
            if(!tex) {
                S_WARN("Couldn't locate texture: {0}", final);
            } else {
                new_mat->set_diffuse_map(tex);
                tex->flush();
            }

            enabled_textures |= DIFFUSE_MAP_ENABLED;
        }

        if(mat.specular_map[0]) {
            Path final = Path(std::string(mat.specular_map, 32).c_str());
            if(!mesh_opts.override_texture_extension.empty()) {
                final = final.replace_ext(mesh_opts.override_texture_extension);
            }

            auto tex = mesh->asset_manager().load_texture(final);
            if(!tex) {
                S_WARN("Couldn't locate texture: {0}", final);
            } else {
                new_mat->set_specular_map(tex);
                tex->flush();
            }

            enabled_textures |= SPECULAR_MAP_ENABLED;
        }

        if(mat.light_map[0]) {
            Path final = Path(std::string(mat.light_map, 32).c_str());
            if(!mesh_opts.override_texture_extension.empty()) {
                final = final.replace_ext(mesh_opts.override_texture_extension);
            }

            auto tex = mesh->asset_manager().load_texture(final);
            if(!tex) {
                S_WARN("Couldn't locate texture: {0}", final);
            } else {
                new_mat->set_light_map(tex);
                tex->flush();
            }

            enabled_textures |= LIGHT_MAP_ENABLED;
        }

        if(mat.normal_map[0]) {
            Path final = Path(std::string(mat.normal_map, 32).c_str());
            if(!mesh_opts.override_texture_extension.empty()) {
                final = final.replace_ext(mesh_opts.override_texture_extension);
            }

            auto tex = mesh->asset_manager().load_texture(final);
            if(!tex) {
                S_WARN("Couldn't locate texture: {0}", final);
            } else {
                new_mat->set_normal_map(tex);
                tex->flush();
            }

            enabled_textures |= NORMAL_MAP_ENABLED;
        }

        new_mat->set_textures_enabled(enabled_textures);

        materials.push_back(new_mat);
    }

    if(added) {
        smlt::get_app()->vfs->remove_search_path(filename_.parent());
    }

    MeshHeader mheader;
    read_data_header(&mheader.data_header);
    data_->read((char*) &mheader.submesh_count, sizeof(mheader.submesh_count));
    data_->read((char*) &mheader.vertex_count, sizeof(mheader.vertex_count));

    auto vdata = mesh->vertex_data.get();
    vdata->move_to_start();
    for(uint32_t i = 0; i < mheader.vertex_count; ++i) {
        auto posa = spec.attr(VERTEX_ATTR_NAME_POSITION).value().arrangement;

        auto tex0 = spec.attr(VERTEX_ATTR_NAME_TEXCOORD_0).value().arrangement;
        auto norm = spec.attr(VERTEX_ATTR_NAME_NORMAL).value().arrangement;

        auto col = spec.attr(VERTEX_ATTR_NAME_COLOR).value().arrangement;

        if(posa == VERTEX_ATTR_ARRANGEMENT_XY) {
            Vec2 v;
            data_->read((char*) &v, sizeof(Vec2));
            vdata->position(v);
        } else if(posa == VERTEX_ATTR_ARRANGEMENT_XYZ) {
            Vec3 v;
            data_->read((char*) &v, sizeof(Vec3));
            vdata->position(v);
        } else if(posa == VERTEX_ATTR_ARRANGEMENT_XYZW) {
            Vec4 v;
            data_->read((char*) &v, sizeof(Vec4));
            vdata->position(v);
        }

        if(tex0 == VERTEX_ATTR_ARRANGEMENT_XY) {
            Vec2 v;
            data_->read((char*) &v, sizeof(Vec2));
            vdata->tex_coord0(v);
        }

        if(col == VERTEX_ATTR_ARRANGEMENT_RGBA &&
           spec.attr(VERTEX_ATTR_NAME_COLOR).value().type ==
               VERTEX_ATTR_TYPE_UNSIGNED_BYTE) {
            uint8_t color[4];
            data_->read((char*) &color, sizeof(color));
            vdata->color(smlt::Color::from_bytes(color[0], color[1], color[2],
                                                 color[3]));
        } else if(col == VERTEX_ATTR_ARRANGEMENT_RGBA) {
            float color[4];
            data_->read((char*) &color, sizeof(color));
            vdata->color(smlt::Color(color[0], color[1], color[2], color[3]));
        } else if(col == VERTEX_ATTR_ARRANGEMENT_RGB) {
            float color[3];
            data_->read((char*) &color, sizeof(color));
            vdata->color(smlt::Color(color[0], color[1], color[2], 1.0f));
        }

        if(norm == VERTEX_ATTR_ARRANGEMENT_XYZ) {
            Vec3 v;
            data_->read((char*) &v, sizeof(Vec3));
            vdata->normal(v);
        }

        vdata->move_next();
    }

    vdata->done();

    for(int i = 0; i < mheader.submesh_count; ++i) {
        SubMeshHeader sheader;
        read_data_header(&sheader.data_header);

        data_->read((char*) &sheader.material_id, sizeof(sheader.material_id));
        data_->read((char*) &sheader.arrangement, sizeof(sheader.arrangement));
        data_->read((char*) &sheader.type, sizeof(sheader.type));
        data_->read((char*) &sheader.num_ranges_or_indices, sizeof(sheader.num_ranges_or_indices));

        auto arrangement = (sheader.arrangement == SUB_MESH_ARRANGEMENT_TRIANGLES) ?
            MESH_ARRANGEMENT_TRIANGLES : MESH_ARRANGEMENT_TRIANGLE_STRIP;

        if(sheader.type == SUB_MESH_TYPE_RANGED) {
            auto sm = mesh->create_submesh(smlt::to_string(i), materials[sheader.material_id], arrangement);
            for(int j = 0; j < sheader.num_ranges_or_indices; ++j) {
                SubMeshVertexRange range;
                data_->read((char*) &range.start, sizeof(uint32_t));
                data_->read((char*) &range.count, sizeof(uint32_t));

                sm->add_vertex_range(range.start, range.count);
            }
        } else {
            auto type = (fheader.index_size == 1) ?
                INDEX_TYPE_8_BIT : (fheader.index_size == 2) ?
                INDEX_TYPE_16_BIT : INDEX_TYPE_32_BIT;

            auto index_data = std::make_shared<IndexData>(type);
            for(int j = 0; j < sheader.num_ranges_or_indices; ++j) {
                if(type == INDEX_TYPE_8_BIT) {
                    uint8_t idx;
                    data_->read((char*) &idx, sizeof(uint8_t));
                    index_data->index(idx);
                } else if(type == INDEX_TYPE_16_BIT) {
                    uint16_t idx;
                    data_->read((char*) &idx, sizeof(uint16_t));
                    index_data->index(idx);
                } else {
                    uint32_t idx;
                    data_->read((char*) &idx, sizeof(uint32_t));
                    index_data->index(idx);
                }
            }

            mesh->create_submesh(smlt::to_string(i), materials[sheader.material_id], index_data, arrangement);
        }
    }

    for(auto& sm: mesh->each_submesh()) {
        sm->mark_changed();
    }
}

}
}
