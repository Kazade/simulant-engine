#include "dcm_loader.h"
#include "../application.h"
#include "../asset_manager.h"
#include "../meshes/mesh.h"
#include "../utils/limited_string.h"
#include "../utils/pbr.h"
#include "../vertex_data.h"
#include "../vfs.h"
#include "dcm.h"
#include <cstdint>

namespace smlt {
namespace loaders {

using namespace dcm;

VertexSpecification determine_spec(const FileHeader& header) {
    /* FIXME:
     * - Support short UV format
     * - Support 3UB color format
     */
    VertexSpecification vspec;
    vspec.position_attribute = (header.pos_format == POSITION_FORMAT_2F) ?
        VERTEX_ATTRIBUTE_2F : (header.pos_format == POSITION_FORMAT_3F) ?
        VERTEX_ATTRIBUTE_3F : VERTEX_ATTRIBUTE_4F;
    vspec.texcoord0_attribute = (header.tex0_format == TEX_COORD_FORMAT_2F) ? VERTEX_ATTRIBUTE_2F : VERTEX_ATTRIBUTE_NONE;
    vspec.texcoord1_attribute = (header.tex1_format == TEX_COORD_FORMAT_2F) ? VERTEX_ATTRIBUTE_2F : VERTEX_ATTRIBUTE_NONE;
    vspec.color_attribute =
        (header.color_format == COLOR_FORMAT_4UB)  ? VERTEX_ATTRIBUTE_4UB_RGBA
        : (header.color_format == COLOR_FORMAT_3F) ? VERTEX_ATTRIBUTE_3F
                                                   : VERTEX_ATTRIBUTE_4F;
    vspec.normal_attribute = (header.normal_format == NORMAL_FORMAT_3F) ? VERTEX_ATTRIBUTE_3F : VERTEX_ATTRIBUTE_NONE;

    /* FIXME: Do something better! */
#if defined(__ANDROID__) || defined(__LINUX__)
    if(vspec.color_attribute == VERTEX_ATTRIBUTE_4UB_RGBA) {
        vspec.color_attribute = VERTEX_ATTRIBUTE_4F;
    }
#elif defined(__DREAMCAST__)
    if(vspec.color_attribute == VERTEX_ATTRIBUTE_4UB_RGBA) {
        vspec.color_attribute = VERTEX_ATTRIBUTE_4UB_BGRA;
    }
#endif

    return vspec;
}

bool DCMLoader::into(Loadable& resource, const LoaderOptions& options) {
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
        return false;
    }

    LimitedString<3> hid((const char*)fheader.id);

    if(hid != std::string("DCM")) {
        S_ERROR("Not a valid .dcm file: {0}", fheader.id);
        return false;
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

        data_->read((char*) mat.base_color_map, sizeof(mat.base_color_map));
        data_->read((char*) mat.light_map, sizeof(mat.light_map));
        data_->read((char*) mat.normal_map, sizeof(mat.normal_map));
        data_->read((char*) mat.metallic_roughness_map, sizeof(mat.metallic_roughness_map));

        smlt::MaterialPtr new_mat =
            mesh->asset_manager().clone_default_material();

        new_mat->set_lighting_enabled(true);
        new_mat->set_cull_mode(mesh_opts.cull_mode);
        new_mat->set_blend_func(mesh_opts.blending_enabled ? BLEND_ALPHA : BLEND_NONE);

        auto s = traditional_to_pbr(
            smlt::Color(mat.ambient, 4), smlt::Color(mat.diffuse, 4),
            smlt::Color(mat.specular, 4), mat.shininess * 128.0f);

        new_mat->set_base_color(s.base_color);
        new_mat->set_metallic(s.metallic);
        new_mat->set_roughness(s.roughness);
        new_mat->set_specular_color(s.specular_color);
        new_mat->set_specular(s.specular);

        new_mat->set_name(
            std::string(mat.data_header.path, sizeof(mat.data_header.path))
                .c_str());

        int enabled_textures = 0;

        if(mat.base_color_map[0]) {
            Path final = Path(std::string(mat.base_color_map, 32).c_str());
            if(!mesh_opts.override_texture_extension.empty()) {
                final = final.replace_ext(mesh_opts.override_texture_extension);
            }

            auto tex = mesh->asset_manager().load_texture(final);
            if(!tex) {
                S_WARN("Couldn't locate texture: {0}", final);
            } else {
                new_mat->set_base_color_map(tex);
                tex->flush();
            }

            enabled_textures |= BASE_COLOR_MAP_ENABLED;
        }

        if(mat.metallic_roughness_map[0]) {
            Path final = Path(std::string(mat.metallic_roughness_map, 32).c_str());
            if(!mesh_opts.override_texture_extension.empty()) {
                final = final.replace_ext(mesh_opts.override_texture_extension);
            }

            auto tex = mesh->asset_manager().load_texture(final);
            if(!tex) {
                S_WARN("Couldn't locate texture: {0}", final);
            } else {
                new_mat->set_metallic_roughness_map(tex);
                tex->flush();
            }

            enabled_textures |= METALLIC_ROUGHNESS_MAP_ENABLED;
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
        if(spec.position_attribute == VERTEX_ATTRIBUTE_2F) {
            Vec2 v;
            data_->read((char*) &v, sizeof(Vec2));
            vdata->position(v);
        } else if(spec.position_attribute == VERTEX_ATTRIBUTE_3F) {
            Vec3 v;
            data_->read((char*) &v, sizeof(Vec3));
            vdata->position(v);
        } else if(spec.position_attribute == VERTEX_ATTRIBUTE_4F) {
            Vec4 v;
            data_->read((char*) &v, sizeof(Vec4));
            vdata->position(v);
        }

        if(spec.texcoord0_attribute == VERTEX_ATTRIBUTE_2F) {
            Vec2 v;
            data_->read((char*) &v, sizeof(Vec2));
            vdata->tex_coord0(v);
        }

        if(fheader.color_format == COLOR_FORMAT_4UB) {
            uint8_t color[4];
            data_->read((char*) &color, sizeof(color));
            vdata->color(smlt::Color::from_bytes(color[0], color[1], color[2],
                                                 color[3]));
        } else if(fheader.color_format == COLOR_FORMAT_4F) {
            float color[4];
            data_->read((char*) &color, sizeof(color));
            vdata->color(smlt::Color(color[0], color[1], color[2], color[3]));
        } else if(fheader.color_format == COLOR_FORMAT_3F) {
            float color[3];
            data_->read((char*) &color, sizeof(color));
            vdata->color(smlt::Color(color[0], color[1], color[2], 1.0f));
        }

        if(spec.normal_attribute == VERTEX_ATTRIBUTE_3F) {
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

    return true;
}
}
}
