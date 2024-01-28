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
    vspec.diffuse_attribute = (header.color_format == COLOR_FORMAT_4UB) ?
        VERTEX_ATTRIBUTE_4UB : (header.color_format == COLOR_FORMAT_3F) ?
        VERTEX_ATTRIBUTE_3F : VERTEX_ATTRIBUTE_4F;
    vspec.normal_attribute = (header.normal_format == NORMAL_FORMAT_3F) ? VERTEX_ATTRIBUTE_3F : VERTEX_ATTRIBUTE_NONE;

    /* FIXME: Do something better! */
#if defined(__ANDROID__) || defined(__LINUX__)
    if(vspec.diffuse_attribute == VERTEX_ATTRIBUTE_4UB) {
        vspec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;
    }
#endif

    return vspec;
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

    for(int i = 0; i < fheader.material_count; ++i) {
        dcm::Material mat;
        data_->read((char*)&mat, sizeof(dcm::Material));

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
    data_->read((char*) &mheader, sizeof(MeshHeader));

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
            vdata->diffuse(smlt::Color::from_bytes(color[2], color[1], color[0], color[3]));
        } else if(fheader.color_format == COLOR_FORMAT_4F) {
            float color[4];
            data_->read((char*) &color, sizeof(color));
            vdata->diffuse(smlt::Color(color[0], color[1], color[2], color[3]));
        } else if(fheader.color_format == COLOR_FORMAT_3F) {
            float color[3];
            data_->read((char*) &color, sizeof(color));
            vdata->diffuse(smlt::Color(color[0], color[1], color[2], 1.0f));
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
        data_->read((char*) &sheader, sizeof(SubMeshHeader));

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
